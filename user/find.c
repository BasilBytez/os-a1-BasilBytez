#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
// Write the matcher as a small recursive function and test it on its own.
void find (char *path)
{
	char buf[512], *p;
	int fd; // fd - file descriptor - int representing kernel managed object that a process may read from or write to
	struct dirent de; // de - directory entry
	struct stat st; // st - stat : a file details and type 

	if ((fd = open(path, O_RDONLY)) < 0) {
    	fprintf(2, "find: cannot open %s\n", path);
    	return;
	}

	if (fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	// do an ls command - either by rewriting code or calling it
		// depends if i can handle the I/O directly
	 
	//while(true)
	int true = 1;
	while (true == 1)
	{
		if (path == T_DIR) printf ("directory found");
		else fprint(path);

	}
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
