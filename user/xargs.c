// xargs.c - run a command once per line of stdin (or once per N words),
// with the words from stdin tacked onto the end of the command's args
// usage: xargs [-n N] cmd [args...]

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"   // MAXARG (32)

// longest single word we'll take from stdin
#define MAXWORD 512

// the argv we hand to exec for each run:
//   cmd, its own args, the words from stdin, then a 0 to end the list
// exec only has room for MAXARG pointers *including* that 0,
// so one run can have at most MAXARG - 1 real arguments
char *args[MAXARG];
int base_count;   // slots used by cmd + its own args (these stay the same every run)
int arg_count;    // slots filled right now

// the word we're in the middle of reading
// (global so the buffer isn't sitting on the small stack)
char word[MAXWORD];
int word_len = 0;

int failed = 0;   // gets set to 1 if any run exits with a non-zero status

// print usage to stderr and exit 1
void
usage(void)
{
  fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
  exit(1);
}

// 1 if s is all digits (and not empty), otherwise 0
int
all_digits(char *s)
{
  if (s[0] == '\0')
    return 0;
  for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] < '0' || s[i] > '9')
      return 0;
  }
  return 1;
}

// put a copy of the word we just finished reading on the end of args
void
add_word(void)
{
  if (arg_count == MAXARG - 1) {
    fprintf(2, "xargs: too many arguments\n");
    exit(1);
  }

  word[word_len] = '\0';               // end the string

  // the word buffer gets reused for the next word, so save a copy
  char *copy = malloc(word_len + 1);
  if (copy == 0) {
    fprintf(2, "xargs: out of memory\n");
    exit(1);
  }
  strcpy(copy, word);

  args[arg_count] = copy;
  arg_count = arg_count + 1;
  word_len = 0;                        // ready for the next word
}

// run the command with everything in args and wait for it to finish,
// then clear out the stdin words so the next run starts fresh
void
run(void)
{
  args[arg_count] = 0;   // exec needs the list to end with a 0

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "xargs: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // child: turn into the command. exec only comes back if it failed
    exec(args[0], args);
    fprintf(2, "xargs: cannot run %s\n", args[0]);
    exit(1);
  }

  // parent: wait for the child and see how it went
  int status = 0;
  wait(&status);
  if (status != 0)
    failed = 1;

  // free the stdin words (cmd and its args point into argv, so not those)
  for (int i = base_count; i < arg_count; i++)
    free(args[i]);
  arg_count = base_count;
}

int
main(int argc, char *argv[])
{
  int n = 0;          // the -n value, or 0 if there's no -n (one run per line)
  int cmd_index = 1;  // where cmd is in argv

  // optional -n N at the front
  if (argc >= 2 && strcmp(argv[1], "-n") == 0) {
    if (argc < 3 || !all_digits(argv[2]))
      usage();
    n = atoi(argv[2]);
    if (n < 1)
      usage();
    cmd_index = 3;
  }

  // there has to be a command after the options
  if (cmd_index >= argc)
    usage();

  // copy cmd and its own args into the front of args
  base_count = argc - cmd_index;
  if (base_count > MAXARG - 1) {
    fprintf(2, "xargs: too many arguments\n");
    exit(1);
  }
  for (int i = 0; i < base_count; i++)
    args[i] = argv[cmd_index + i];
  arg_count = base_count;

  // read stdin one char at a time
  char c;
  while (read(0, &c, 1) == 1) {

    // part of a word: save it and keep going
    if (c != ' ' && c != '\t' && c != '\n') {
      if (word_len == MAXWORD - 1) {
        fprintf(2, "xargs: word too long\n");
        exit(1);
      }
      word[word_len] = c;
      word_len = word_len + 1;
      continue;
    }

    // a space, tab or newline ends whatever word we were reading
    if (word_len > 0) {
      add_word();
      // with -n: run as soon as we have N words
      if (n > 0 && arg_count - base_count == n)
        run();
    }

    // without -n: the end of a line means run, if the line had any words
    if (c == '\n' && n == 0 && arg_count > base_count)
      run();
  }

  // end of input: the last word might not have had a space or newline after it
  if (word_len > 0)
    add_word();

  // anything left over gets one last run (with -n it can be fewer than N)
  if (arg_count > base_count)
    run();

  exit(failed);
}
