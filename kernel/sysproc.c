#include "types.h"
#include "fcntl.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

#define FAIL_ADDR 0xffffffffffffffff
uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


int
check_aligned(uint64 addr)
{
  if(addr % PGSIZE == 0)
    return 1;

  return 0;
}

uint64
sys_mmap(void)
{
  uint64 length;
  int prot, flags, fd;
  uint64 oldsz;
  struct vma *v;

  // Get arguments from the stack, do the valid check.
  if(argaddr(1, &length) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0 ||
      argint(4, &fd) < 0){
    printf("invalid argument!\n");
    return FAIL_ADDR;
  }

  // check read only file get mapped write permission.
  if(flags & MAP_SHARED) {
    // ensure that the prot is less than the open mode.
    if (!myproc()->ofile[fd]->writable && (prot & PROT_WRITE))
      return FAIL_ADDR;
  }
  // Set up the vma area, and added to the process.
  v = (struct vma *)get_vma();
  v -> starting_addr = myproc()->sz;
  if (check_aligned(v->starting_addr) == 0){
    printf("addr:%p\n", v->starting_addr);
    panic("mmap: not aligned");
  }
  v -> length = length;
  v -> f = myproc()->ofile[fd];
  v -> prot = prot;
  v -> flags = flags;
  v -> next = myproc()->vma_area;
  myproc()->vma_area = v;


  // Increase the opened file count.
  filedup(myproc()->ofile[fd]);


  // Increase the process size.
  oldsz = myproc()->sz;
  myproc()->sz += length;

  return oldsz;
}

// This function should free the section and write to the underlying file if needed.
void
munmap_pages(uint64 addr, int page_count, int flags, struct vma *v)
{
  printf("addr is:%p\n", addr);
  printf("free %d pages\n", page_count);
  struct proc *p = myproc();
  int write_back;

  if (flags & MAP_SHARED)
    write_back = 1;
  else
    write_back = 0;

  if(write_back == 1){
    // write the content back to the file.
    filewrite_kernel(v->f, addr, page_count * PGSIZE, addr - v->starting_addr);
  }
  // unmap the content
  uvmunmap(p->pagetable, addr, page_count, 1);
}

uint64
sys_munmap(void)
{
  // TODO: Reduce the file ref count.
  uint64 addr, length;
  int unmap_pages, total_pages, available_pages;
  struct vma *v;
  if (argaddr(0, &addr) < 0 || argaddr(1, &length) < 0)
    return -1;

  // check that addr is page-aligned.
  if(check_aligned(addr) == 0)
    panic("unmap: not aligned address");

  // find the vma related to the address.
  v = find_vma(addr); 
  // How many pages should we unmap?
  if ((v->starting_addr + v->length - addr) % PGSIZE != 0) {
    available_pages = (v->starting_addr + v->length - addr) / PGSIZE + 1;
  } else {
    available_pages = (v->starting_addr + v->length - addr) / PGSIZE;
  }
  if (length % PGSIZE != 0){
    unmap_pages = length / PGSIZE + 1;
  } else {
    unmap_pages = length / PGSIZE;
  }
  if (v->length % PGSIZE != 0) {
    total_pages = v->length / PGSIZE + 1;
  } else {
    total_pages = v->length / PGSIZE;
  }
  // new added 
  if (unmap_pages > available_pages)
    unmap_pages = available_pages;

  if (addr == v->starting_addr) {
    if (unmap_pages > total_pages) {
      panic("unmap: more pages unmmaped than total");
    }
    if (unmap_pages == total_pages) {
      munmap_pages(addr, unmap_pages, v->flags, v);
      // close the related file.
      fileclose(v->f); 
      // Release the vma pointer.
      free_vma(v);
    } else {
      munmap_pages(addr, unmap_pages, v->flags, v);
      v->starting_addr = v->starting_addr + PGSIZE * unmap_pages;
      v->length -= unmap_pages * PGSIZE;
    }
  } else {
    munmap_pages(addr, unmap_pages, v->flags, v);
    v->length -= unmap_pages * PGSIZE;
  }
  return 0;
}
