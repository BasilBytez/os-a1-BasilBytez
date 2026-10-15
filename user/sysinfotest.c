// sysinfotest.c - checks the sysinfo system call
// does the checks the handout lists: freemem goes down when memory is
// allocated, nproc goes up after fork, and nopenfile changes with
// open, pipe and close. also checks that a bad pointer gives -1

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int failures = 0;

// call sysinfo, and give up right away if it fails
void
get(struct sysinfo *info)
{
  if (sysinfo(info) < 0) {
    printf("FAIL: sysinfo returned -1 on a good pointer\n");
    exit(1);
  }
}

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

int
main(int argc, char *argv[])
{
  struct sysinfo before, after;

  // 1. the call works and the numbers look sane
  get(&before);
  printf("freemem %lu bytes, nproc %lu, nopenfile %lu\n",
         before.freemem, before.nproc, before.nopenfile);
  check(before.freemem > 0 && before.freemem % 4096 == 0,
        "freemem is a positive multiple of 4096");
  check(before.nproc >= 3, "nproc counts at least init, sh and this test");

  // 2. freemem goes down when memory is allocated
  get(&before);
  sbrk(10 * 4096);              // grab 10 pages (sbrk allocates right away)
  get(&after);
  check(after.freemem <= before.freemem - 10 * 4096,
        "freemem drops by at least 10 pages after sbrk");
  sbrk(-10 * 4096);             // give them back
  get(&before);
  check(before.freemem > after.freemem, "freemem goes back up after freeing them");

  // 3. nproc goes up after fork, and back down once the child is gone
  get(&before);
  int pid = fork();
  if (pid == 0) {
    pause(10);                  // child: stick around for a second, then quit
    exit(0);
  }
  get(&after);
  check(after.nproc == before.nproc + 1, "nproc goes up by 1 after fork");
  wait(0);
  get(&after);
  check(after.nproc == before.nproc, "nproc goes back down after wait");

  // 4. nopenfile changes with open, pipe and close
  get(&before);
  int fd = open("README", O_RDONLY);
  get(&after);
  check(after.nopenfile == before.nopenfile + 1, "open adds 1 open file");
  close(fd);
  get(&after);
  check(after.nopenfile == before.nopenfile, "close takes it away again");

  int p[2];
  get(&before);
  pipe(p);
  get(&after);
  check(after.nopenfile == before.nopenfile + 2, "pipe adds 2 open files (both ends)");
  close(p[0]);
  close(p[1]);
  get(&after);
  check(after.nopenfile == before.nopenfile, "closing both ends takes them away");

  // 5. bad pointers get -1 instead of crashing anything
  char *past_end = sbrk(0) + 100 * 4096;   // past the end of our memory
  check(sysinfo((struct sysinfo *)past_end) < 0, "pointer past the end of memory gives -1");
  check(sysinfo((struct sysinfo *)0xffffffffffUL) < 0, "pointer past the biggest address gives -1");

  if (failures == 0) {
    printf("sysinfotest: OK\n");
    exit(0);
  }
  printf("sysinfotest: %d FAILED\n", failures);
  exit(1);
}
