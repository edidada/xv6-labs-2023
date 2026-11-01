#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"
#include "linked_list.h"

struct link {
  int value;
  struct link *next;
};

// For simplicity, we only allocate one page.
char *base;
char *p;

struct link *l;

struct link *ll_new() {
  if (base == 0 || p + sizeof(struct link) > base + PGSIZE) {
    return 0;
  }

  struct link *ret = (struct link *)p;
  p += sizeof(struct link);
  return ret;
}

void ll_create() { if(base) kfree(base); l = 0; base = kalloc(); p = base; }

int ll_push(int value) {
  struct link *new = ll_new();
  if (new == 0) {
    return -1;
  }

  if (l == 0) {
    l = new;
  } else {
    struct link *end;
    for (end = l; end->next != 0; end = end->next)
      ;
    end->next = new;
  }
  new->next = 0;
  new->value = value;
  return 0;
}

int ll_pop(void) {
  if (l == 0) {
    return DUMMY_VALUE;
  }

  struct link *old = l;
  l = l->next;
  // Now let's clean up and make it harder to misuse.
  old->next = 0;
  return old->value;
}

void ll_destroy(void) { if(base) kfree(base); base = 0; p = 0; l = 0; }
