// find.c -- search a folder tree for names that match a pattern.
//
// Usage: find <dir> <pattern> [-type f|d]
//
// We're building this in segments:
//   Segment 1 (this version): check the arguments, then print the
//                             full path of every entry in ONE folder.
//   Segment 2: also search every folder below it.
//   Segment 3: only print names that match the pattern.
//   Segment 4: the -type f / -type d filter.

#include "kernel/types.h"   // uint, ushort, ... (must come first)
#include "kernel/stat.h"    // struct stat, T_DIR, T_FILE, T_DEVICE
#include "user/user.h"      // open, read, close, printf, malloc, ...
#include "kernel/fs.h"      // struct dirent, DIRSIZ
#include "kernel/fcntl.h"   // O_RDONLY

// =====================================================================
// Settings from the command line. main() fills these in.
// They're global so every function can read them.
// =====================================================================
char *pattern;          // the name pattern to look for, e.g. "*.c"
char type_filter = 0;   // 0 = no -type given, 'f' = files only, 'd' = folders only

// =====================================================================
// usage: print how to run find, then quit with exit status 1.
// =====================================================================
void
usage(void)
{
  fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
  exit(1);
}

// =====================================================================
// make_path: build a brand-new string "dir/name" and return it.
//   make_path("a", "y.c")    -> "a/y.c"
//   make_path("a/", "y.c")   -> "a/y.c"    (no doubled '/')
//   make_path("/", "README") -> "/README"
// The string comes from malloc, so whoever gets it must free() it.
// =====================================================================
char *
make_path(char *dir, char *name)
{
  int dir_len = strlen(dir);
  int name_len = strlen(name);

  // Room for: dir + '/' + name + the ending '\0'
  char *path = malloc(dir_len + 1 + name_len + 1);
  if (path == 0) {
    fprintf(2, "find: out of memory\n");
    exit(1);
  }

  // Start with the folder part, e.g. "a"
  strcpy(path, dir);

  // Add a '/' after it, unless it already ends with one
  if (dir_len > 0 && dir[dir_len - 1] != '/') {
    path[dir_len] = '/';
    dir_len = dir_len + 1;
  }

  // Add the name right after, e.g. "a/" + "y.c" -> "a/y.c"
  strcpy(path + dir_len, name);

  return path;
}

// =====================================================================
// search_folder: look at every entry directly inside one folder.
// (Segment 1: just print each entry's full path.)
// =====================================================================
void
search_folder(char *path)
{
  int fd;              // file descriptor for the open folder
  struct dirent de;    // one record from the folder: inode number + name
  struct stat st;      // details about the folder itself

  // --- 1. Open the folder ---
  fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  // --- 2. Make sure it really is a folder ---
  // fstat fills in st; st.type says file, folder or device.
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  if (st.type != T_DIR) {
    // A file or device has no entries inside it: nothing to do.
    close(fd);
    return;
  }

  // --- 3. Read the folder one 16-byte record at a time ---
  // read() returns 16 while records are left, and 0 at the end.
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {

    // Skip empty slots (inode number 0).
    if (de.inum == 0)
      continue;

    // Copy the name into a normal C string. A name that uses all 14
    // characters has no ending '\0' in the record, so we add one.
    char name[DIRSIZ + 1];
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = '\0';

    // Skip "." (this folder) and ".." (its parent).
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    // Build the entry's full path, e.g. "a" + "y.c" -> "a/y.c"
    char *child = make_path(path, name);

    // SEGMENT 1 TEST: print every entry. (Later segments change this.)
    printf("%s\n", child);

    // Done with this path, so give its memory back.
    free(child);
  }

  // --- 4. Done with this folder ---
  close(fd);
}

// =====================================================================
// main: check the arguments, then start the search.
// =====================================================================
int
main(int argc, char *argv[])
{
  // --- 1. Right number of arguments? ---
  //   find <dir> <pattern>           -> argc is 3
  //   find <dir> <pattern> -type f   -> argc is 5
  if (argc != 3 && argc != 5)
    usage();

  // --- 2. Remember the pattern ---
  pattern = argv[2];

  // --- 3. With 5 arguments, the last two must be "-type f" or "-type d" ---
  if (argc == 5) {
    if (strcmp(argv[3], "-type") != 0)
      usage();
    if (strcmp(argv[4], "f") == 0)
      type_filter = 'f';
    else if (strcmp(argv[4], "d") == 0)
      type_filter = 'd';
    else
      usage();
  }

  // --- 4. The starting folder must open. The handout requires this
  //        exact message and exit status 1 if it doesn't. ---
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", argv[1]);
    exit(1);
  }
  close(fd);   // we only opened it to check; search_folder opens it again

  // --- 5. Search ---
  search_folder(argv[1]);   // Segment 2 changes this line
  exit(0);
}

/*
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"


// Returns 1 if every byte of the record is zero (an empty slot), else 0.
int
all_zero(struct dirent *d)
{
  char *bytes = (char *)d;            // look at the record as raw bytes
  for (int i = 0; i < sizeof(*d); i++) {
    if (bytes[i] != 0)
      return 0;                       // found a non-zero byte: not empty
  }
  return 1;                           // all 16 bytes were zero
}


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
	
	// de.inum -> holds record of inode data	
	
	while (read(fd, &de, sizeof(de)) == sizeof(de)) {
		// skips empty slots (inode number 0),
		
		if (all_zero(&de))
			continue;                         // empty slot: go to the next record

//  -------- TO IMPLEMENT: TYPE CHECKING INSIDE OF SWITCH STATEMENT -------
//		// checks TYPE.  If directory skips
//		switch (st.type) {
//			case T_DEVICE:
//			
//			case T_FILE:
//			
//			case T_DIR:
//				// need to print THEN search recursively
//		}
//		
//		bool empty = true;
		for(int i = 0; i < 16; i++){
			if(read(fd[i] != 0)) empty = false; 
			break;
		}
		

		// skips empty slots '.'
 		
		// skips empty slots '..' 
		if (empty == false);
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
	1. Skip empty slots (inode number 0), . and ...

	2. Build the entry’s full path, such as a/b.

	3. Call stat on that path to learn its type.

	4. If the name matches the pattern and passes the -type filter, print the path.

	5. If it’s a folder, call find on that path.

After the loop: close(fd)
---- Two things around the loop need attention as well: ----

Only run the loop for folders. Use st.type to check, so a file like README never gets read as records.
Close fd after the loop. Right now find returns without closing it. Each call to find opens one file descriptor, and a process gets only 16, three of which are already taken. Without close, find would fail after visiting about 13 folders in total, not just 13 levels deep.


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
*/
