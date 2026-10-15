// status.c - test helper (not part of the assignment)
// runs a command and prints its exit status, since xv6's shell never shows it
// usage: status cmd [args...]

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "usage: status cmd [args...]\n");
    exit(1);
  }

  int pid = fork();
  if (pid == 0) {
    exec(argv[1], argv + 1);   // run the command with the rest of the args
    fprintf(2, "status: cannot run %s\n", argv[1]);
    exit(1);
  }

  int st = 0;
  wait(&st);
  printf("[exit status %d]\n", st);
  exit(0);
}
