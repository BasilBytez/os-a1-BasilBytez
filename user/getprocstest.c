// getprocstest.c - checks the getprocs system call
// covers: normal call, max smaller than the number of processes,
// max = 0, max < 0, a bad pointer, fork, and a fork/exit stress test

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/uproc.h"
#include "user/user.h"

struct uproc buf[NPROC + 1];   // global so it's not on the small stack
int failures = 0;

// print ok/FAIL for one check and count the failures
void
check(int passed, char *what)
{
  if (passed) {
    printf("ok:   %s\n", what);
  } else {
    printf("FAIL: %s\n", what);
    failures = failures + 1;
  }
}

// find pid in the first n entries of buf, or return -1
int
find_pid(int n, int pid)
{
  for (int i = 0; i < n; i++) {
    if (buf[i].pid == pid)
      return i;
  }
  return -1;
}

int
main(int argc, char *argv[])
{
  int me = getpid();

  // 1. normal call: init, sh and us at least, and our own entry looks right
  int n = getprocs(buf, NPROC);
  check(n >= 3, "getprocs finds at least 3 processes");
  check(buf[0].pid == 1 && buf[0].ppid == 0, "first entry is init, with ppid 0");
  int i = find_pid(n, me);
  check(i >= 0, "our own pid is in the list");
  if (i >= 0) {
    check(buf[i].state == 4, "we show up as running");
    check(strcmp(buf[i].name, "getprocstest") == 0, "our name is right");
  }

  // 2. max smaller than the number of processes: stops after max,
  //    and doesn't write past it
  buf[1].pid = 12345;   // marker, should still be here afterwards
  check(getprocs(buf, 1) == 1, "max = 1 returns 1");
  check(buf[1].pid == 12345, "max = 1 doesn't write a second entry");
  check(getprocs(buf, 0) == 0, "max = 0 returns 0");

  // 3. bad arguments
  check(getprocs(buf, -1) == -1, "max < 0 returns -1");
  check(getprocs((struct uproc *)0xffffffffffUL, NPROC) == -1, "bad pointer returns -1");

  // 4. after fork the child shows up, with us as its parent
  int pid = fork();
  if (pid == 0) {
    pause(10);
    exit(0);
  }
  int n2 = getprocs(buf, NPROC);
  check(n2 == n + 1, "one more process after fork");
  int c = find_pid(n2, pid);
  check(c >= 0 && buf[c].ppid == me, "child is listed with our pid as ppid");
  wait(0);

  // 5. stress: children fork and exit over and over while we keep
  //    calling getprocs. bad locking tends to panic or hang here
  int kids[4];
  for (int k = 0; k < 4; k++) {
    kids[k] = fork();
    if (kids[k] == 0) {
      for (int j = 0; j < 50; j++) {
        int g = fork();
        if (g == 0)
          exit(0);
        if (g > 0)
          wait(0);
      }
      exit(0);
    }
  }
  int bad = 0;
  for (int j = 0; j < 200; j++) {
    int m = getprocs(buf, NPROC);
    if (m < 1 || m > NPROC)
      bad = 1;
  }
  for (int k = 0; k < 4; k++)
    wait(0);
  check(bad == 0, "200 calls during fork/exit stress all look sane");

  if (failures == 0) {
    printf("getprocstest: OK\n");
    exit(0);
  }
  printf("getprocstest: %d FAILED\n", failures);
  exit(1);
}
