// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock steal_lock;
  struct bucket bucket[NBUCKET];
  struct buf buf[NBUF];
  struct buf head;
} bcache;

char *bcache_lock_names[NBUCKET] = {
  "bcache0",
  "bcache1",
  "bcache2",
  "bcache3",
  "bcache4",
  "bcache5",
  "bcache6",
  "bcache7",
  "bcache8",
  "bcache9",
  "bcache10",
  "bcache11",
  "bcache12",
};

static uint
bhash(uint dev, uint blockno)
{
  return (blockno + dev) % NBUCKET;
}

static void
bremove(struct buf *b)
{
  b->next->prev = b->prev;
  b->prev->next = b->next;
}

static void
binsert(uint bucket, struct buf *b)
{
  struct buf *head = &bcache.bucket[bucket].head;

  b->bucket = bucket;
  b->next = head->next;
  b->prev = head;
  head->next->prev = b;
  head->next = b;
}


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.steal_lock, "bcache.steal");

  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.bucket[i].lock, bcache_lock_names[i]);
    bcache.bucket[i].head.prev = &bcache.bucket[i].head;
    bcache.bucket[i].head.next = &bcache.bucket[i].head;
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    acquire(&bcache.bucket[(b - bcache.buf) % NBUCKET].lock);
    binsert((b - bcache.buf) % NBUCKET, b);
    release(&bcache.bucket[b->bucket].lock);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bucket = bhash(dev, blockno);

  acquire(&bcache.bucket[bucket].lock);

  // Is the block already cached?
  for(b = bcache.bucket[bucket].head.next;
      b != &bcache.bucket[bucket].head;
      b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket[bucket].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Recycle an unused buffer from this bucket if possible.
  for(b = bcache.bucket[bucket].head.prev;
      b != &bcache.bucket[bucket].head;
      b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucket[bucket].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.bucket[bucket].lock);

  // Serializing recycling preserves the one-copy-per-block invariant.
  acquire(&bcache.steal_lock);
  acquire(&bcache.bucket[bucket].lock);

  // Another CPU may have inserted this block while we waited.
  for(b = bcache.bucket[bucket].head.next;
      b != &bcache.bucket[bucket].head;
      b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket[bucket].lock);
      release(&bcache.steal_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  for(b = bcache.bucket[bucket].head.prev;
      b != &bcache.bucket[bucket].head;
      b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucket[bucket].lock);
      release(&bcache.steal_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket[bucket].lock);

  for(int i = 0; i < NBUCKET; i++){
    if(i == bucket)
      continue;

    acquire(&bcache.bucket[i].lock);
    for(b = bcache.bucket[i].head.prev;
        b != &bcache.bucket[i].head;
        b = b->prev){
      if(b->refcnt == 0) {
        bremove(b);
        release(&bcache.bucket[i].lock);

        acquire(&bcache.bucket[bucket].lock);
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        binsert(bucket, b);
        release(&bcache.bucket[bucket].lock);
        release(&bcache.steal_lock);

        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.bucket[i].lock);
  }

  release(&bcache.steal_lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.bucket[b->bucket].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    bremove(b);
    binsert(b->bucket, b);
  }
  
  release(&bcache.bucket[b->bucket].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.bucket[b->bucket].lock);
  b->refcnt++;
  release(&bcache.bucket[b->bucket].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.bucket[b->bucket].lock);
  b->refcnt--;
  release(&bcache.bucket[b->bucket].lock);
}


