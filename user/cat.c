#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[512];

void
// fd == file descriptor -> see pg 11 of xv6 manual
cat(int fd)
{
  int n;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      fprintf(2, "cat: write error\n");
      exit(1);
    }
  }
  if (n < 0) {
    fprintf(2, "cat: read error\n");
    exit(1);
  }
}

// When you type cat README notes, the shell sets argc to 3, argv[0] to "cat", argv[1] to "README" and argv[2] to "notes".
int
main(int argc, char *argv[])
{
  int fd, i;

  if (argc <= 1) {
    cat(0);
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], O_RDONLY)) < 0) {
      fprintf(2, "cat: cannot open %s\n", argv[i]);
      exit(1);
	// Q: What is the difference between fprintf and printf.c in the xv6/user folder?
	// A: 
	/* 
		Both functions do the same formatting; they differ only in where the text goes. In printf.c, both just call vprintf:

		printf(...) always passes file descriptor 1, which is standard output.
		fprintf(fd, ...) lets you choose the file descriptor.
	*/
	// use printf for normal output
	// fprintf for error and usage messages
    }
    cat(fd);
    close(fd);
  }
  exit(0);
}
