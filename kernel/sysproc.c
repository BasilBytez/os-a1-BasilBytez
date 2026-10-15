#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "sysinfo.h"

// sysinfo(struct sysinfo *info): fill in a struct sysinfo and copy it
// out to the user's address. returns 0, or -1 if the copy fails
uint64
sys_sysinfo(void)
{
  uint64 user_addr;         // where the user wants the struct written
  struct sysinfo info;      // our copy, here in kernel memory
  struct proc *p = myproc();

  argaddr(0, &user_addr);   // first argument = the user's pointer

  info.freemem = count_freemem();
  info.nproc = count_processes();
  info.nopenfile = count_openfiles();

  // copyout in this xv6 takes 5 args: page table, the process size
  // (p->sz), the user address, the kernel data, and how many bytes.
  // it fails (returns -1) if the user address is bad
  if (copyout(p->pagetable, p->sz, user_addr, (char *)&info, sizeof(info)) < 0)
    return -1;

  return 0;
}

// trace(int mask): trace the syscalls whose bits are set in mask.
// the mask lives in struct proc, so kfork copies it to children
// and exec leaves it alone
uint64
sys_trace(void)
{
  int mask;
  argint(0, &mask);
  myproc()->trace_mask = mask;
  return 0;
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
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
