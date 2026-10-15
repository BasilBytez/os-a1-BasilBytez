// sleep.c - pause for a number of timer ticks
// usage: sleep <ticks>   (one tick is about 1/10 of a second in QEMU)

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// print usage to stderr and exit 1
void
usage(void)
{
  fprintf(2, "usage: sleep <ticks>\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  // need exactly one argument (argv[0] is "sleep" itself)
  if (argc != 2)
    usage();

  // it has to be all digits. atoi doesn't check that on its own:
  // atoi("12x") gives 12 and atoi("abc") gives 0. checking for digits
  // also rules out negative numbers, since '-' isn't a digit
  char *s = argv[1];
  if (s[0] == '\0')
    usage();
  for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] < '0' || s[i] > '9')
      usage();
  }

  pause(atoi(s));
  exit(0);
}
