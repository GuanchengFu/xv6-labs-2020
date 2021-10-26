#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

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

uint64
sys_munmap(void)
{
  printf("sys_munmap get called!\n");
  // TODO: Implement this.
  // TODO: Reduce the file ref count.
  return -1;
}
