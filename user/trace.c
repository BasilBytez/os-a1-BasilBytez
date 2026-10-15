// trace.c - turn on syscall tracing, then run a command with it on
// usage: trace <mask> <cmd> [args...]
// bit i of mask set = trace syscall number i, e.g. 32800 = open + read

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  // need at least a mask and a command
  if (argc < 3) {
    fprintf(2, "usage: trace <mask> <cmd> [args...]\n");
    exit(1);
  }

  // atoi doesn't handle a minus sign, so do that part ourselves
  // (so something like "trace -1 ..." still turns on every bit)
  int mask;
  if (argv[1][0] == '-')
    mask = -atoi(argv[1] + 1);
  else
    mask = atoi(argv[1]);

  if (trace(mask) < 0) {
    fprintf(2, "trace: trace failed\n");
    exit(1);
  }

  // become the command. the mask stays on through exec,
  // and any children it forks inherit it
  exec(argv[2], &argv[2]);

  // exec only comes back if it failed
  fprintf(2, "trace: exec %s failed\n", argv[2]);
  exit(1);
}
