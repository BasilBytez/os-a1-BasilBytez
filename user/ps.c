// ps.c - list every process: pid, parent pid, state, memory size, name
// usage: ps

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"   // NPROC (64)
#include "kernel/uproc.h"
#include "user/user.h"

// global, not a local: 64 entries is 2.5KB and the user stack is only 4KB
struct uproc procs[NPROC];

// turn a state number into its name. same order as
// enum procstate in kernel/proc.h: UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE
char *
state_name(int state)
{
  if (state == 1)
    return "used";
  if (state == 2)
    return "sleeping";
  if (state == 3)
    return "runnable";
  if (state == 4)
    return "running";
  if (state == 5)
    return "zombie";
  return "unknown";   // shouldn't happen, getprocs skips UNUSED
}

int
main(int argc, char *argv[])
{
  int n = getprocs(procs, NPROC);
  if (n < 0) {
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }

  // fields separated by a single tab, like the handout
  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");
  for (int i = 0; i < n; i++) {
    printf("%d\t%d\t%s\t%lu\t%s\n", procs[i].pid, procs[i].ppid,
           state_name(procs[i].state), procs[i].sz, procs[i].name);
  }

  exit(0);
}
