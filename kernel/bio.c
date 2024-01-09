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

#define BucketSize 17
// 结构体的定义发生在编译时期，而const int变量是在运行时期才有确定的值。
// 因此，编译器无法在编译时期确定结构体数组的大小，因为它依赖于在运行时期才能得到的const int变量的值。
// const int BucketSize = 13;
struct {
  struct spinlock lock;
  struct buf buf[MAXOPBLOCKS];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache[BucketSize];

void
binit(void)
{
    struct buf *b;
    for (int i = 0; i < BucketSize; ++i) {
        initlock(&bcache[i].lock, "bcache");
        // Create linked list of buffers
        bcache[i].head.prev = &bcache[i].head;
        bcache[i].head.next = &bcache[i].head;
        for(b = bcache[i].buf; b < bcache[i].buf+MAXOPBLOCKS; b++){
            b->next = bcache[i].head.next;
            b->prev = &bcache[i].head;
            initsleeplock(&b->lock, "buffer");
            bcache[i].head.next->prev = b;
            bcache[i].head.next = b;
        }
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = (dev + blockno) % BucketSize;
  acquire(&bcache[id].lock);

  // Is the block already cached?
  for(b = bcache[id].head.next; b != &bcache[id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache[id].head.prev; b != &bcache[id].head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
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
  int id = (b->dev + b->blockno) % BucketSize;
  acquire(&bcache[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache[id].head.next;
    b->prev = &bcache[id].head;
    bcache[id].head.next->prev = b;
    bcache[id].head.next = b;
  }
  
  release(&bcache[id].lock);
}

void
bpin(struct buf *b) {
    int id = (b->dev + b->blockno) % BucketSize;
  acquire(&bcache[id].lock);
  b->refcnt++;
  release(&bcache[id].lock);
}

void
bunpin(struct buf *b) {
    int id = (b->dev + b->blockno) % BucketSize;
  acquire(&bcache[id].lock);
  b->refcnt--;
  release(&bcache[id].lock);
}


