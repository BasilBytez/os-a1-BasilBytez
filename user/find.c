// find.c -- search a folder tree for names that match a pattern.
//
// Usage: find <dir> <pattern> [-type f|d]
//
// We're building this in segments:
//   Segment 1: check the arguments, then print the full path of
//              every entry in ONE folder.
//   Segment 2 (this version): also search every folder below it.
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
// The to-do list: folders we still need to search.
// New folders are added at the end, and we work through the list from
// the front, one folder at a time. Because nothing calls itself, the
// search can go as deep as needed without running out of stack.
// xv6's disk holds fewer than 200 files and folders in total, so a
// list of 200 is always big enough.
// =====================================================================
#define MAX_FOLDERS 200
char *todo[MAX_FOLDERS];   // the folder paths (each one made with malloc)
int todo_count = 0;        // how many folders have been added so far
int todo_next = 0;         // position of the next folder to search

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
// add_folder: put a folder's path at the end of the to-do list.
// The list takes over the path: find() frees it after searching it.
// =====================================================================
void
add_folder(char *path)
{
  if (todo_count == MAX_FOLDERS) {
    // Can't happen on xv6's disk, but never write past the list's end.
    fprintf(2, "find: too many folders, skipping %s\n", path);
    free(path);
    return;
  }
  todo[todo_count] = path;
  todo_count = todo_count + 1;
}

// =====================================================================
// search_folder: look at every entry directly inside one folder.
// (Segment 2: print each entry's full path, and put subfolders on
// the to-do list so they get searched later.)
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

    // Find out what kind of entry it is: file, folder or device.
    // The record itself doesn't say, so we ask stat().
    struct stat child_st;
    if (stat(child, &child_st) < 0) {
      fprintf(2, "find: cannot stat %s\n", child);
      free(child);
      continue;
    }

    // SEGMENT 2 TEST: print every entry. (Segments 3 and 4 add checks.)
    printf("%s\n", child);

    // A folder goes on the to-do list to be searched later. The list
    // now owns `child`, so we must NOT free it here.
    // Anything else is finished, so free its path now.
    if (child_st.type == T_DIR)
      add_folder(child);
    else
      free(child);
  }

  // --- 4. Done with this folder ---
  close(fd);
}

// =====================================================================
// find: search the starting folder and every folder below it.
// =====================================================================
void
find(char *start)
{
  // Put a copy of the starting folder on the list. Every path on the
  // list gets freed when we're done with it, so it must come from
  // malloc, not straight from argv.
  char *first = malloc(strlen(start) + 1);
  if (first == 0) {
    fprintf(2, "find: out of memory\n");
    exit(1);
  }
  strcpy(first, start);
  add_folder(first);

  // Work through the list from the front. search_folder() adds new
  // folders to the end as it finds them, so this loop keeps going
  // until every folder that was ever added has been searched.
  while (todo_next < todo_count) {
    char *folder = todo[todo_next];   // the next folder to search
    todo_next = todo_next + 1;        // move our place in the list along

    search_folder(folder);

    free(folder);                     // finished with this folder's path
  }
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
  find(argv[1]);
  exit(0);
}
