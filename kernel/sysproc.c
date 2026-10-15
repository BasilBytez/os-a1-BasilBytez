#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

#include "sysinfo.h"

// Add this function to handle the system call.
uint64
sys_sysinfo(void)
{
  uint64 user_info_addr;
  struct sysinfo local_info;
  struct proc *p = myproc(); // Get the current process[cite: 2]

  // Retrieve the first argument passed to the system call, which is the 
  // user-space address where we need to copy the struct[cite: 1].
  argaddr(0, &user_info_addr);

  // Call our helper functions to populate the local struct.
  local_info.freemem = count_freemem();
  local_info.nproc = count_processes();
  local_info.nopenfile = count_openfiles();

  // Use copyout to safely move data from kernel memory to user memory[cite: 1].
  // copyout takes: (process page table, destination address, source address, size)
  if (copyout(p->pagetable, user_info_addr, (char *)&local_info, sizeof(local_info)) < 0) {
    // If the copy fails (e.g., invalid user address), return -1[cite: 1].
    return -1;
  }

  // Return 0 on success[cite: 1].
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
