#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// The call read(fd, buf, n) reads at most n bytes from the file descriptor

// Write the matcher as a small recursive function and test it on its own.
void find (char *path)
{
//	char buf[512], *p;
	int fd; // fd - file descriptor - int representing kernel managed object that a process may read from or write to
	struct dirent de; // de - directory entry
	struct stat st; // st - stat : a file details and type 

// - - - returns error if path cannot be opened - - - //
	// sets file descriptor to path
	fd = open(path, O_RDONLY); // O_RDONLY -> instructs open to open file for reading
						       // int open(char *file, int flags)
							   // Open a file; flags indicate read/write; returns an fd (file descriptor).	
	if (fd < 0){
    	fprintf(2, "find: cannot open %s\n", path);
    	return;
	}

// - - - returns error if stat cannot be used on file descriptor - - - //
	// checks if stat can be used on file descriptor
	int stat_error = fstat(fd, &st);
	if (stat_error < 0) { 
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

// - - - visits every record in the folder, one per pass, and stops when there are none left. - - - //
	// The call read(fd, buf, n) reads at most n bytes from the file descriptor fd, copies them into buf, and returns the number of bytes read.
	while (read(fd, &de, sizeof(de)) == sizeof(de)) {

		printf("inode\tname\n");                 // once, before the loop
		printf("%d\t[%s]\n", de.inum, de.name);  // inside the loop
												 // name and inode numbers need to match 3rd column of 'ls' command
												 // SKIP: '.' '..' and empty slots (full of zeros)
			
	}	 

// TEST mkdir COMMAND
// mkdir a; mkdir a/b; echo > a/b/x.c; echo > a/y.c
//		echo > a/y.c creates an empty file. 
//		echo with nothing after it prints nothing, and > sends that nothing into a new file called a/y.c.
//
// 		Check with:
//				ls
//				ls a/b
//
// 		Then run fprint statements

// < < < WHILE LOOP PSEUDOCODE > > > //
/* 	
	1. Skip empty slots (inode number 0), . and ...

	2. Build the entry’s full path, such as a/b.

	3. Call stat on that path to learn its type.

	4. If the name matches the pattern and passes the -type filter, print the path.

	5. If it’s a folder, call find on that path.

After the loop: close(fd)
---- Two things around the loop need attention as well: ----

Only run the loop for folders. Use st.type to check, so a file like README never gets read as records.
Close fd after the loop. Right now find returns without closing it. Each call to find opens one file descriptor, and a process gets only 16, three of which are already taken. Without close, find would fail after visiting about 13 folders in total, not just 13 levels deep.
*/


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
