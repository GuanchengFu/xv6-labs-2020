// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

/*struct {*/
  /*struct spinlock lock;*/
  /*struct run *freelist;*/
/*} kmem;*/

// NCPU is 8, which is the maximum number of CPU allowed in this machine.
struct {
  struct spinlock lock;
  struct run *freelist;
  int core_num;
} kmem_c[NCPU];

void
kinit()
{
  /*initlock(&kmem.lock, "kmem");*/
  for (int i = 0; i < NCPU; i ++) {
    initlock(&kmem_c[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  int i = 0;
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    kfree(p, i);
    i += 1;
    i = i % NCPU;
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// core_num is which CPU executes this code.
void
kfree(void *pa, int core_num)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem_c[core_num].lock);
  r->next = kmem_c[core_num].freelist;
  kmem_c[core_num].freelist = r;
  release(&kmem_c[core_num].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// This is where we should keep the information of no memory error.
void *
kalloc(int core_num)
{
  struct run *r;

  acquire(&kmem_c[core_num].lock);
  // TODO: Error handling when there is no memory inside!
  r = kmem_c[core_num].freelist;
  if(r)
    kmem_c[core_num].freelist = r->next;
  release(&kmem_c[core_num].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
