/* rephrase.c (the main program)
 * Copyright (C) 2003  Phil Lanch
 *
 * This file is part of Rephrase.
 *
 * Rephrase is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * Rephrase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* $Id: rephrase.c,v 1.1 2003/09/03 05:02:24 phil Exp $ */

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define PROGRAM "rephrase"
#ifndef VERSION
#error VERSION must be defined
#endif

#ifndef GPG
#define GPG "/usr/local/bin/gpg"
#endif

#ifndef PATTERN_MAX
#define PATTERN_MAX 512
#endif
#define ALTERNATIVES_MAX ((PATTERN_MAX + 1) / 2)

const char LF = '\n';

struct secrets {
  char pattern[PATTERN_MAX + 1];
  int alternatives[ALTERNATIVES_MAX];
  int try[ALTERNATIVES_MAX];
  int i, a, b, alt_n;
  short is_alt, is_literal, error;
  ssize_t io_count;
};

void
read_pattern (struct secrets *s)
{
  FILE *tty_fp;
  struct termios term_save, term;
  int pattern_err;

  if (!(tty_fp = fopen ("/dev/tty", "r+"))) {
    perror ("fopen: ");
    exit (8);
  }
  if (tcgetattr (fileno (tty_fp), &term_save)) {
    perror ("tcgetattr: ");
    exit (9);
  }
  term = term_save;
  term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
  if (tcsetattr (fileno (tty_fp), TCSAFLUSH, &term)) {
    perror ("(1st) tcsetattr: ");
    exit (10);
  }

  fprintf (tty_fp, "Enter pattern: ");
  fflush (tty_fp);
  for (s->i = 0, pattern_err = 0; s->i <= PATTERN_MAX; ++s->i) {
    while (!(s->io_count = read (fileno (tty_fp), s->pattern + s->i, 1))) {
      sleep (1);
    }
    if (s->io_count == -1) {
      fprintf (tty_fp, "\n");
      perror ("read: ");
      pattern_err = 11;
      break;
    }
    if (s->pattern[s->i] == '\n') {
      s->pattern[s->i] = '\0';
      fprintf (tty_fp, "\n");
      break;
    }
  }
  if (s->i > PATTERN_MAX) {
    do {
      while (!(s->io_count = read (fileno (tty_fp), s->pattern, 1))) {
        sleep (1);
      }
      if (s->io_count == -1) {
        fprintf (tty_fp, "\n");
        perror ("read: ");
        pattern_err = 11;
        break;
      }
    } while (*s->pattern != '\n');
    if (!pattern_err) {
      fprintf (tty_fp, "\n");
      fprintf (stderr, "Pattern is too long\n(maximum length is %d;"
          " you could redefine PATTERN_MAX and recompile)\n", PATTERN_MAX);
      pattern_err = 12;
    }
  }

  if (tcsetattr (fileno (tty_fp), TCSAFLUSH, &term_save)) {
    perror ("(2nd) tcsetattr: ");
    exit (pattern_err ? pattern_err : 13);
  }
  if (pattern_err) {
    exit (pattern_err);
  }
}

void
parse_pattern (struct secrets *s)
{
  for (s->i = s->a = s->is_alt = s->error = 0; s->pattern[s->i] && !s->error;
          ++s->i) {
    switch (s->pattern[s->i]) {
      case '\\': ++s->i;
                 if (!s->pattern[s->i]) {
                   s->error = 1;
                 }
                 break;
      case '(':  if (s->is_alt) {
                   s->error = 1;
                 } else {
                   s->is_alt = 1;
                   s->alternatives[s->a] = 0;
                 }
                 break;
      case '|':  if (!s->is_alt) {
                   s->error = 1;
                 } else {
                   ++s->alternatives[s->a];
                 }
                 break;
      case ')':  if (!s->is_alt) {
                   s->error = 1;
                 } else {
                   s->is_alt = 0;
                   ++s->a;
                 }
                 break;
    }
  }
  if (s->error || s->is_alt) {
    fprintf (stderr, "Pattern is malformed\n");
    exit (14);
  }

  for (s->b = 0; s->b < s->a; ++s->b ) {
    s->try[s->b] = 0;
  }
}

void
spawn_gpg (const char *key, int dev_null, int *pass_writer, pid_t *kid)
{
  int pass_fds[2];
  char s_pass_reader[21];

  if (pipe (pass_fds)) {
    perror ("pipe: ");
    exit (16);
  }
  *pass_writer = pass_fds[1];
  snprintf (s_pass_reader, sizeof (s_pass_reader), "%d", pass_fds[0]);

  if ((*kid = fork ()) == -1) {
    perror ("fork: ");
    exit (17);
  }

  if (!*kid) {
    if (close (pass_fds[1])) {
      perror ("(kid) close: ");
      exit (18);
    }
    if (dup2 (dev_null, 0) == -1 || dup2 (dev_null, 1) == -1 
        || dup2 (dev_null, 2) == -1) {
      perror ("(kid) dup2: ");
      exit (19);
    }
    execl (GPG, "gpg", "--default-key", key, "--passphrase-fd", s_pass_reader,
        "--batch", "--dry-run", "--clearsign", "/dev/null", NULL);
    perror ("(kid) execlp: ");
    exit (20);
  }

  if (close (pass_fds[0])) {
    perror ("(parent) close: ");
    exit (21);
  }
}

void
write_passphrase (struct secrets *s, int pass_writer)
{
  for (s->i = s->b = 0; s->pattern[s->i]; ++s->i) {
    switch (s->pattern[s->i]) {
      case '\\': ++s->i;
                 s->is_literal = 1;
                 break;
      case '(':  s->is_alt = 1;
                 s->alt_n = 0;
                 s->is_literal = 0;
                 break;
      case '|':  ++s->alt_n;
                 s->is_literal = 0;
                 break;
      case ')':  s->is_alt = 0;
                 ++s->b;
                 s->is_literal = 0;
                 break;
      default:   s->is_literal = 1;
                 break;
    }
    if (s->is_literal && (!s->is_alt || s->alt_n == s->try[s->b])) {
      while (!(s->io_count = write (pass_writer, s->pattern + s->i, 1))) {
        sleep (1);
      }
      if (s->io_count == -1) {
        perror ("write: ");
        exit (22);
      }
    }
  }
  while (!(s->io_count = write (pass_writer, &LF, 1))) {
    sleep (1);
  }
  if (s->io_count == -1) {
    perror ("(last) write: ");
    exit (23);
  }

  if (close (pass_writer)) {
    perror ("(final) close: ");
    exit (24);
  }
}

int
passphrase_is_correct (const char *key, struct secrets *s, int dev_null)
{
  int pass_writer;
  pid_t kid;
  int status;

  spawn_gpg (key, dev_null, &pass_writer, &kid);

  write_passphrase (s, pass_writer);

  if (waitpid (kid, &status, 0) == -1) {
    perror ("waitpid: ");
    exit (25);
  }
  if (!WIFEXITED (status)) {
    fprintf (stderr, "gpg didn't exit normally");
    exit (26);
  }
  return (WEXITSTATUS (status) == 0);
}

int
find_passphrase (const char *key, struct secrets *s)
{
  int dev_null;

  if ((dev_null = open ("/dev/null", O_RDWR)) == -1) {
    perror ("open: ");
    exit (15);
  }

  do {
    if (passphrase_is_correct (key, s, dev_null)) {
      fprintf (stderr, "Passphrase found\n");
      for (s->b = 0; s->b < s->a; ++s->b) {
        printf (s->b ? " %d" : "%d", s->try[s->b] + 1);
      }
      printf ("\n");
      return (0);
    }

    s->error = 1;
    for (s->b = s->a - 1; s->b >= 0; --s->b) {
      if (s->try[s->b] < s->alternatives[s->b]) {
        ++s->try[s->b];
        for (s->i = s->b + 1; s->i < s->a; ++s->i) {
          s->try[s->i] = 0;
        }
        s->error = 0;
        break;
      }
    }
  } while (!s->error);

  fprintf (stderr, "Passphrase doesn't match pattern (or no such key)\n");
  return (1);
}

int
main (int argc, char **argv)
{
  struct secrets sec;
  struct stat stat_buf;

  fprintf (stderr, "%s (Rephrase) %s\nCopyright (C) 2003  Phil Lanch\n"
      "This program comes with ABSOLUTELY NO WARRANTY.\n"
      "This is free software, and you are welcome to redistribute it\n"
      "under certain conditions.  See the file COPYING for details.\n\n",
      PROGRAM, VERSION);

  if (mlock (&sec, sizeof (struct secrets))) {
    perror ("mlock: ");
    fprintf (stderr, "(%s should be installed setuid root)\n", PROGRAM);
    exit (2);
  }
  if (setreuid (getuid (), getuid ())) {
    perror ("setreuid: ");
    exit (3);
  }

  if (stat (GPG, &stat_buf)) {
    if (errno & (ENOENT | ENOTDIR)) {
      fprintf (stderr, "%s does not exist (or is in a directory I cannot read)"
          "\n(perhaps you need to redefine GPG and recompile)\n", GPG);
      exit (4);
    }
    perror ("stat: ");
    exit (5);
  }
  if (!S_ISREG(stat_buf.st_mode)
      || !(stat_buf.st_mode & (stat_buf.st_uid == getuid () ? S_IXUSR
      : stat_buf.st_gid == getgid () ? S_IXGRP : S_IXOTH))) {
    fprintf (stderr, "%s is not an executable (by me) file\n", GPG);
    exit (6);
  }

  if (argc != 2) {
    fprintf (stderr, "Usage: %s <key>\n", PROGRAM);
    exit (7);
  }

  read_pattern (&sec);

  parse_pattern (&sec);

  return (find_passphrase (argv[1], &sec));
}
