// find.c - search a directory tree for names that match a pattern
// usage: find <dir> <pattern> [-type f|d]
//
// progress:
//   part 1 - arg checking + listing one dir        (done)
//   part 2 - walk every subdir with a to-do list   (done)
//   part 3 - only print names that match           (this version)
//   part 4 - -type f / -type d filter              (todo)

#include "kernel/types.h"   // uint etc, has to come first
#include "kernel/stat.h"    // struct stat, T_DIR / T_FILE / T_DEVICE
#include "user/user.h"      // syscalls + printf, malloc, strcmp...
#include "kernel/fs.h"      // struct dirent, DIRSIZ (14)
#include "kernel/fcntl.h"   // O_RDONLY

// settings from the command line (main fills these in)
// global so every function can see them
char *pattern;          // name pattern, like "*.c"
char type_filter = 0;   // 0 = no -type, 'f' = files only, 'd' = dirs only

// to-do list of dirs we still need to search.
// new dirs go on the end, and we work through it from the front.
// no recursion this way, so a deep tree can't overflow the 4KB stack.
// xv6's disk only has room for ~200 files/dirs total, so 200 slots is enough
#define MAX_FOLDERS 200
char *todo[MAX_FOLDERS];   // paths (each one malloc'd)
int todo_count = 0;        // how many dirs have been added so far
int todo_next = 0;         // index of the next dir to search

// print usage to stderr and exit with status 1
void
usage(void)
{
  fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
  exit(1);
}

// returns a new malloc'd string "dir/name" -- caller has to free it
//   ("a", "y.c")    -> "a/y.c"
//   ("a/", "y.c")   -> "a/y.c"   (doesn't double up the /)
//   ("/", "README") -> "/README"
char *
make_path(char *dir, char *name)
{
  int dir_len = strlen(dir);
  int name_len = strlen(name);

  // dir + '/' + name + '\0'
  char *path = malloc(dir_len + 1 + name_len + 1);
  if (path == 0) {
    fprintf(2, "find: out of memory\n");
    exit(1);
  }

  strcpy(path, dir);   // "a"

  // add the '/' unless dir already ends in one
  if (dir_len > 0 && dir[dir_len - 1] != '/') {
    path[dir_len] = '/';          // overwrites the '\0' strcpy left there
    dir_len = dir_len + 1;
  }

  strcpy(path + dir_len, name);   // "a/" + "y.c" -> "a/y.c"
  return path;
}

// does name match pat? returns 1 or 0
// '*' matches any run of chars (even none), everything else has to match exactly.
// this one does call itself, but only about once per char of pat/name
// (never more than ~30 deep), so it's fine on the small stack
int
match(char *pat, char *name)
{
  // out of pattern: only a match if we're out of name too
  if (pat[0] == '\0')
    return name[0] == '\0';

  if (pat[0] == '*') {
    // try 1: the * matches nothing, so skip past it
    if (match(pat + 1, name))
      return 1;
    // try 2: the * eats one more char of the name (keep the * for more)
    if (name[0] != '\0' && match(pat, name + 1))
      return 1;
    return 0;
  }

  // normal char: has to be the same as the name's next char
  if (pat[0] == name[0])
    return match(pat + 1, name + 1);

  return 0;
}

// add a dir's path to the end of the to-do list.
// the list owns the path after this, find() frees it once it's searched
void
add_folder(char *path)
{
  if (todo_count == MAX_FOLDERS) {
    // shouldn't happen on xv6, but don't write past the end of the array
    fprintf(2, "find: too many folders, skipping %s\n", path);
    free(path);
    return;
  }
  todo[todo_count] = path;
  todo_count = todo_count + 1;
}

// go through every entry directly inside one dir:
// print the ones whose name matches, and put subdirs on the to-do list
void
search_folder(char *path)
{
  int fd;              // fd for the open dir
  struct dirent de;    // one record: inode number + name
  struct stat st;      // info about the dir itself

  // open it
  fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  // make sure it's actually a dir (files/devices have nothing inside)
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  if (st.type != T_DIR) {
    close(fd);
    return;
  }

  // read one 16 byte record at a time until read() stops giving us a full one
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {

    // empty slot
    if (de.inum == 0)
      continue;

    // copy the name out and null terminate it
    // (a 14 char name has no '\0' in the record)
    char name[DIRSIZ + 1];
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = '\0';

    // skip . and .. or we'd loop forever
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    char *child = make_path(path, name);   // full path, e.g. "a/y.c"

    // the record doesn't say file vs dir, so ask stat
    struct stat child_st;
    if (stat(child, &child_st) < 0) {
      fprintf(2, "find: cannot stat %s\n", child);
      free(child);
      continue;
    }

    // print it if the name matches (just the name, not the whole path)
    // TODO part 4: check -type here too
    if (match(pattern, name))
      printf("%s\n", child);

    // dirs go on the to-do list even if their name didn't match,
    // since stuff inside them still could. the list owns child now,
    // so don't free it. anything else we're done with
    if (child_st.type == T_DIR)
      add_folder(child);
    else
      free(child);
  }

  close(fd);
}

// search start and everything under it
void
find(char *start)
{
  // put a malloc'd copy of start on the list (everything on the list
  // gets freed later, and the argv strings weren't malloc'd)
  char *first = malloc(strlen(start) + 1);
  if (first == 0) {
    fprintf(2, "find: out of memory\n");
    exit(1);
  }
  strcpy(first, start);
  add_folder(first);

  // keep taking the next dir off the list until every dir that got added
  // has been searched. search_folder() adds more to the end as it finds them
  while (todo_next < todo_count) {
    char *folder = todo[todo_next];
    todo_next = todo_next + 1;

    search_folder(folder);

    free(folder);
  }
}

int
main(int argc, char *argv[])
{
  // valid: find <dir> <pattern>          (argc 3)
  //        find <dir> <pattern> -type f  (argc 5)
  if (argc != 3 && argc != 5)
    usage();

  pattern = argv[2];

  // with 5 args the last two have to be "-type f" or "-type d"
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

  // handout: if <dir> can't be opened, print this and exit 1
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", argv[1]);
    exit(1);
  }
  close(fd);   // just checking, search_folder opens it again

  find(argv[1]);
  exit(0);
}
