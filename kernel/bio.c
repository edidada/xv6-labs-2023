// Bucketed block cache: hits lock one bucket, misses serialize eviction.
#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 31
static struct {
  struct spinlock lock;
  struct buf *head;
} buckets[NBUCKET];
static struct {
  struct spinlock eviction;
  struct buf buffers[NBUF];
  uint cursor;
} bcache;

void
binit(void)
{
  initlock(&bcache.eviction, "bcache.eviction");
  for(int i = 0; i < NBUCKET; i++)
    initlock(&buckets[i].lock, "bcache.bucket");
  for(int i = 0; i < NBUF; i++){
    struct buf *b = &bcache.buffers[i];
    initsleeplock(&b->lock, "buffer");
    b->dev = ~0U;
    b->next = buckets[0].head;
    buckets[0].head = b;
  }
}

// The acquired reference prevents eviction while waiting for the sleep lock.
static struct buf *
lookup(uint bucket, uint dev, uint blockno)
{
  for(struct buf *b = buckets[bucket].head; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      return b;
    }
  }
  return 0;
}

static struct buf *
bget(uint dev, uint blockno)
{
  uint target = blockno % NBUCKET;
  acquire(&buckets[target].lock);
  struct buf *b = lookup(target, dev, blockno);
  release(&buckets[target].lock);
  if(b){
    acquiresleep(&b->lock);
    return b;
  }
  acquire(&bcache.eviction);
  acquire(&buckets[target].lock);
  b = lookup(target, dev, blockno);
  release(&buckets[target].lock);
  if(!b){
    // Remove a free buffer while holding its old bucket lock. Only one
    // eviction runs at a time, and no path holds two bucket locks.
    for(uint n = 0; n < NBUCKET && !b; n++){
      uint source = (bcache.cursor + n) % NBUCKET;
      acquire(&buckets[source].lock);
      struct buf **link = &buckets[source].head;
      while(*link){
        if((*link)->refcnt == 0){
          b = *link;
          *link = b->next;
          b->refcnt = 1;
          bcache.cursor = (source + 1) % NBUCKET;
          break;
        }
        link = &(*link)->next;
      }
      release(&buckets[source].lock);
    }
    if(!b)
      panic("bget: no buffers");
    acquire(&buckets[target].lock);
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->next = buckets[target].head;
    buckets[target].head = b;
    release(&buckets[target].lock);
  }
  release(&bcache.eviction);
  acquiresleep(&b->lock);
  return b;
}

struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b = bget(dev, blockno);
  if(!b->valid){
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  releasesleep(&b->lock);
  uint bucket = b->blockno % NBUCKET;
  acquire(&buckets[bucket].lock);
  b->refcnt--;
  release(&buckets[bucket].lock);
}

void
bpin(struct buf *b)
{
  uint bucket = b->blockno % NBUCKET;
  acquire(&buckets[bucket].lock);
  b->refcnt++;
  release(&buckets[bucket].lock);
}

void
bunpin(struct buf *b)
{
  uint bucket = b->blockno % NBUCKET;
  acquire(&buckets[bucket].lock);
  b->refcnt--;
  release(&buckets[bucket].lock);
}
