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

// invariant: Any kernel thread that acquire more than one bucket lock should
// acquire the bcache_lock first.
struct spinlock bcache_lock;

struct {
  struct spinlock lock;
  struct buf buf[NBUF / 5];
  struct buf head;
} bcache_bucket_head[NBUCKET];

void
binit(void)
{
  struct buf *b;

  initlock(&bcache_lock, "bcache");

  for (int i = 0; i < 13; i ++) {
    initlock(&bcache_bucket_head[i].lock, "bcache");
  }

  // Create linked list for different buffers.
  // Create linked list of buffers
  for (int i = 0; i < 13; i ++) {
    bcache_bucket_head[i].head.prev = &bcache_bucket_head[i].head;
    bcache_bucket_head[i].head.next = &bcache_bucket_head[i].head;
    for (b = bcache_bucket_head[i].buf; b < bcache_bucket_head[i].buf + NBUF / 5; b++) {
      b->next = bcache_bucket_head[i].head.next;
      b->prev = &bcache_bucket_head[i].head;
      initsleeplock(&b->lock, "buffer");
      bcache_bucket_head[i].head.next->prev = b;
      bcache_bucket_head[i].head.next = b;
    }
  }

  /*bcache.head.prev = &bcache.head;*/
  /*bcache.head.next = &bcache.head;*/
  /*for(b = bcache.buf; b < bcache.buf+NBUF; b++){*/
    /*b->next = bcache.head.next;*/
    /*b->prev = &bcache.head;*/
    /*initsleeplock(&b->lock, "buffer");*/
    /*bcache.head.next->prev = b;*/
    /*bcache.head.next = b;*/
  /*}*/
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucket_num = blockno % NBUCKET;

  acquire(&bcache_bucket_head[bucket_num].lock);

  // Is the block already cached?
  for (b = bcache_bucket_head[bucket_num].head.next; b != &bcache_bucket_head[bucket_num].head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache_bucket_head[bucket_num].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache_bucket_head[bucket_num].lock);
  // The lock sequence: bcache_lock first, then bucket by bucket.
  acquire(&bcache_lock);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int i;
  int record_bucket = 0, lock_bucket;
  struct buf *record_buf = 0;
  uint least_tick = get_tick();
  /*printf("ticks:%d\n", least_tick);*/
  for(i = 0; i < NBUCKET; i ++) {
    acquire(&bcache_bucket_head[i].lock);
    lock_bucket = i;
    for (b = bcache_bucket_head[i].head.prev; b != &bcache_bucket_head[i].head; b = b->prev) {
      if (b->refcnt == 0) {
        // This is the case where this buffer has never been used.
        if (b->tick == 0) {
          record_buf = b;
          record_bucket = i;
          goto found;
        } else {
          /*printf("b->tick:%d\n", b->tick);*/
          if (b->tick <= least_tick) {
            least_tick = b->tick;
            record_buf = b;
            record_bucket = i;
          }
        }
      }
    }
  }
  if (record_buf == 0) 
    panic("bget: no buffers");
  
found:
  // release the locks that we do not care.
  // i is the source bucket.
  for (int j = lock_bucket; j >= 0; j --) {
    if (j != record_bucket)
      release(&bcache_bucket_head[j].lock);
  }

  // now we need to move the bucket from source to dest.
  if (record_bucket != bucket_num) {
    // acquire the lock for that bucket.
    acquire(&bcache_bucket_head[bucket_num].lock);

    // move the bucket from bucket[record_bucket] to bucket[bucket_num]

    // Delete from bucket[i]
    record_buf -> next -> prev = record_buf->prev;
    record_buf -> prev -> next = record_buf->next;

    record_buf->next = bcache_bucket_head[bucket_num].head.next;
    record_buf->prev = &bcache_bucket_head[bucket_num].head;
    bcache_bucket_head[bucket_num].head.next->prev = record_buf;
    bcache_bucket_head[bucket_num].head.next = record_buf;
    /*printf("bucket i:%d\n", record_bucket);*/
    release(&bcache_bucket_head[record_bucket].lock);
  } else {
    record_buf -> next -> prev = record_buf->prev;
    record_buf -> prev -> next = record_buf->next;

    record_buf->next = bcache_bucket_head[bucket_num].head.next;
    record_buf->prev = &bcache_bucket_head[bucket_num].head;
    bcache_bucket_head[bucket_num].head.next->prev = record_buf;
    bcache_bucket_head[bucket_num].head.next = record_buf;
  }

  // now we can safely release the lock for bcache_lock.
  release(&bcache_lock);
  record_buf->dev = dev;
  record_buf->blockno = blockno;
  record_buf->valid = 0;
  record_buf->refcnt = 1;
  record_buf->tick = get_tick();
  release(&bcache_bucket_head[bucket_num].lock);
  acquiresleep(&record_buf->lock);
  return record_buf;
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

  // we are done with this buffer.
  releasesleep(&b->lock);
  // This is safe, becasue it currently b->refcnt > 0, and nobody can change its states.
  int bucket_num = b->blockno % NBUCKET;
  acquire(&bcache_bucket_head[bucket_num].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache_bucket_head[bucket_num].head.next;
    b->prev = &bcache_bucket_head[bucket_num].head;
    bcache_bucket_head[bucket_num].head.next->prev = b;
    bcache_bucket_head[bucket_num].head.next = b;
  }
  
  release(&bcache_bucket_head[bucket_num].lock);
}

void
bpin(struct buf *b) {
  int bucket_num = b->blockno % NBUCKET;
  acquire(&bcache_bucket_head[bucket_num].lock);
  b->refcnt++;
  release(&bcache_bucket_head[bucket_num].lock);
}

void
bunpin(struct buf *b) {
  int bucket_num = b->blockno % NBUCKET;
  acquire(&bcache_bucket_head[bucket_num].lock);
  b->refcnt--;
  release(&bcache_bucket_head[bucket_num].lock);
}


