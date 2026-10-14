// sleep <ticks> pauses for the given number of timer ticks (one tick is about 1/10 s in QEMU), then exits with status 0.
    // • Use the pause() system call.
    // • If the argument is missing, extra, negative or not all digits, print usage: sleep <ticks> and exit with status 1.

// added test line from PC

// includes xv6 libraries
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[])
{
	// // When you type cat README notes, the shell sets argc to 3, argv[0] to "cat", argv[1] to "README" and argv[2] to "notes".
	if(argc != 1 || atoi(argv[1]) < 0) {
		printf("print usage: sleep <ticks>");
		exit(1);
	}
	// atoi converts char values to int, per '/user/ulib.c'	
	// int time = argv[1] - '0';	
	int time = atoi(argv[1]);
	pause(time);
	exit(0);
}
