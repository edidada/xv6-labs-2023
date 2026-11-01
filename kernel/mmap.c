#include "types.h"
#include "riscv.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

uint64
sys_mmap(void)
{
  uint64 requested;
  int length, prot, flags, fd, offset;
  argaddr(0, &requested);
  argint(1, &length);
  argint(2, &prot);
  argint(3, &flags);
  argint(4, &fd);
  argint(5, &offset);
  struct proc *p = myproc();
  if(requested || length <= 0 || offset != 0 ||
     (prot & ~(PROT_READ | PROT_WRITE)) || prot == 0 ||
     (flags != MAP_SHARED && flags != MAP_PRIVATE) ||
     fd < 0 || fd >= NOFILE)
    return -1;
  struct file *f = p->ofile[fd];
  if(!f || f->type != FD_INODE || !f->readable ||
     (flags == MAP_SHARED && (prot & PROT_WRITE) && !f->writable))
    return -1;
  uint64 top = TRAPFRAME;
  struct vma *slot = 0;
  for(int i = 0; i < NVMA; i++){
    if(p->vmas[i].file){
      if(p->vmas[i].start < top)
        top = p->vmas[i].start;
    } else if(!slot)
      slot = &p->vmas[i];
  }
  uint64 rounded = PGROUNDUP((uint64)length);
  if(!slot || rounded > top || top - rounded < PGROUNDUP(p->sz))
    return -1;
  slot->start = top - rounded;
  slot->length = rounded;
  slot->offset = 0;
  slot->prot = prot;
  slot->flags = flags;
  slot->file = filedup(f);
  return slot->start;
}

int
mmapfault(struct proc *p, uint64 address, int store)
{
  for(int i = 0; i < NVMA; i++){
    struct vma *v = &p->vmas[i];
    if(!v->file || address < v->start || address - v->start >= v->length)
      continue;
    if(store ? !(v->prot & PROT_WRITE) : !(v->prot & PROT_READ))
      return -1;
    uint64 va = PGROUNDDOWN(address);
    pte_t *pte = walk(p->pagetable, va, 0);
    if(pte && (*pte & PTE_V))
      return -1;
    char *mem = kalloc();
    if(!mem)
      return -1;
    memset(mem, 0, PGSIZE);
    ilock(v->file->ip);
    int n = readi(v->file->ip, 0, (uint64)mem,
                  v->offset + va - v->start, PGSIZE);
    iunlock(v->file->ip);
    // RISC-V does not permit write-only leaf PTEs.
    int permissions = PTE_U | PTE_R;
    if(v->prot & PROT_WRITE)
      permissions |= PTE_W;
    if(n < 0 || mappages(p->pagetable, va, PGSIZE,
                         (uint64)mem, permissions) < 0){
      kfree(mem);
      return -1;
    }
    return 0;
  }
  return -1;
}

static void
discard(struct proc *p, uint64 start, uint64 length)
{
  for(uint64 va = start; va < start + length; va += PGSIZE){
    pte_t *pte = walk(p->pagetable, va, 0);
    if(pte && (*pte & PTE_V))
      uvmunmap(p->pagetable, va, 1, 1);
  }
  sfence_vma();
}

int
mmapunmap(struct proc *p, uint64 start, uint64 length)
{
  if(start % PGSIZE || length == 0 || length > MAXVA ||
     start >= MAXVA || length > MAXVA - start)
    return -1;
  length = PGROUNDUP(length);
  for(int i = 0; i < NVMA; i++){
    struct vma *v = &p->vmas[i];
    if(!v->file || start < v->start || start >= v->start + v->length ||
       length > v->start + v->length - start ||
       (start != v->start && start + length != v->start + v->length))
      continue;
    if(v->flags == MAP_SHARED && (v->prot & PROT_WRITE)){
      for(uint64 va = start; va < start + length; va += PGSIZE){
        pte_t *pte = walk(p->pagetable, va, 0);
        if(!pte || !(*pte & PTE_V))
          continue;
        // Keep each write below the file system transaction limit.
        for(uint j = 0; j < PGSIZE; j += BSIZE){
          uint64 off = v->offset + va - v->start + j;
          begin_op();
          ilock(v->file->ip);
          uint n = off < v->file->ip->size ? v->file->ip->size - off : 0;
          if(n > BSIZE)
            n = BSIZE;
          int written = n ? writei(v->file->ip, 0, PTE2PA(*pte) + j, off, n) : 0;
          iunlock(v->file->ip);
          end_op();
          if(written != n)
            return -1;
        }
      }
    }
    discard(p, start, length);
    if(length == v->length){
      struct file *f = v->file;
      memset(v, 0, sizeof *v);
      fileclose(f);
    } else {
      if(start == v->start){
        v->start += length;
        v->offset += length;
      }
      v->length -= length;
    }
    return 0;
  }
  return -1;
}

uint64
sys_munmap(void)
{
  uint64 start;
  int length;
  argaddr(0, &start);
  argint(1, &length);
  if(length <= 0)
    return -1;
  return mmapunmap(myproc(), start, length);
}

void
mmapclose(struct proc *p)
{
  for(int i = 0; i < NVMA; i++){
    struct vma *v = &p->vmas[i];
    if(v->file && mmapunmap(p, v->start, v->length) < 0){
      // Exit must reclaim mappings even if writeback fails.
      discard(p, v->start, v->length);
      struct file *f = v->file;
      memset(v, 0, sizeof *v);
      fileclose(f);
    }
  }
}

int
mmapcopy(struct proc *parent, struct proc *child)
{
  for(int i = 0; i < NVMA; i++){
    struct vma *v = &parent->vmas[i];
    if(!v->file)
      continue;
    child->vmas[i] = *v;
    filedup(v->file);
    // Preserve private modifications already resident at fork time.
    for(uint64 va = v->start; va < v->start + v->length; va += PGSIZE){
      pte_t *pte = walk(parent->pagetable, va, 0);
      if(!pte || !(*pte & PTE_V))
        continue;
      char *mem = kalloc();
      if(!mem)
        goto fail;
      memmove(mem, (void *)PTE2PA(*pte), PGSIZE);
      if(mappages(child->pagetable, va, PGSIZE,
                  (uint64)mem, PTE_FLAGS(*pte)) < 0){
        kfree(mem);
        goto fail;
      }
    }
  }
  return 0;
fail:
  for(int i = 0; i < NVMA; i++){
    struct vma *v = &child->vmas[i];
    if(v->file){
      discard(child, v->start, v->length);
      fileclose(v->file);
      memset(v, 0, sizeof *v);
    }
  }
  return -1;
}
