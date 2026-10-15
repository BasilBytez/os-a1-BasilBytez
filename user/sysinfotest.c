#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct sysinfo info;

  // Call your new system call and pass the address of our struct
  if (sysinfo(&info) < 0) {
    printf("sysinfo test failed: system call returned an error\n");
    exit(1);
  }

  // Print out the system statistics
  // We cast to int because xv6's basic printf handles standard %d best, 
  // and these numbers easily fit within standard integer limits.
  printf("Free memory: %d bytes\n", (int)info.freemem);
  printf("Active processes: %d\n", (int)info.nproc);
  printf("Open files: %d\n", (int)info.nopenfile);

  exit(0);
}
