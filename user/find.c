#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
// Write the matcher as a small recursive function and test it on its own.
void find (....)
{
	// do an ls command - either by rewriting code or calling it
		// depends if i can handle the I/O directly
	while(
	// while for loop -> go to index, 0, 1, 2, 3, etc
	// create match index
	// if (filename == pattern) -> adds to match index
	// if (type == directory) -> enter, reset index A to 0; store current position in index B.
		// so two for loops?
}

// find <dir> <pattern> [-type f|d] searches <dir> recursively and prints the path of every entry whose name matches <pattern>, one path per line. 

int
main (int argc, char *argv[])
{
	int i;

	// find <dir> <pattern> [-type f|d]	
	if (argc > 4 || argc < 3){
		printf("usage: find <dir> <pattern> [-type f|d]");
		exit(1);
	}
	// may need to adjust for loop conditions
	for (i = 0; i < argc; i++)
		find(argv[i]);
	exit(0);
}
