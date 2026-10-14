// sleep <ticks> pauses for the given number of timer ticks (one tick is about 1/10 s in QEMU), then exits with status 0.
    // • Use the pause() system call.
    // • If the argument is missing, extra, negative or not all digits, print usage: sleep <ticks> and exit with status 1.

// includes xv6 libraries
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
sleep(int time)
{

}

int main(int argc, char *argv[])
{
	if(argc <= 1){
	
	}		

	
}
