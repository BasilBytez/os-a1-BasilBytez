// mkdeep.c -- TEST HELPER for find (not part of the assignment).
//
// Usage: mkdeep <top> <levels>
// Makes the folder <top> with <levels> nested "d" folders inside it
// (<top>/d/d/.../d), plus an empty file deep.c in the bottom folder.
// Example: mkdeep t 30

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(2, "usage: mkdeep <top> <levels>\n");
    exit(1);
  }
  int levels = atoi(argv[2]);

  // Make the top folder and step into it.
  if (mkdir(argv[1]) < 0 || chdir(argv[1]) < 0) {
    fprintf(2, "mkdeep: cannot create %s\n", argv[1]);
    exit(1);
  }

  // Make one "d" folder per level and step into each one. Working one
  // level at a time keeps every name short, so we never hit xv6's
  // 128-character path limit while building.
  for (int i = 0; i < levels; i++) {
    if (mkdir("d") < 0 || chdir("d") < 0) {
      fprintf(2, "mkdeep: failed at level %d\n", i + 1);
      exit(1);
    }
  }

  // Leave an empty file at the bottom.
  int fd = open("deep.c", O_CREATE | O_WRONLY);
  if (fd < 0) {
    fprintf(2, "mkdeep: cannot create deep.c\n");
    exit(1);
  }
  close(fd);

  printf("made %s with %d levels\n", argv[1], levels);
  exit(0);
}
