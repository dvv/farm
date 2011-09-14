#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include "ipsvd_check.h"
#include "sgetopt.h"
#include "error.h"
#include "open.h"
#include "lock.h"
#include "strerr.h"
#include "stralloc.h"
#include "direntry.h"
#include "cdb.h"
#include "cdb_make.h"
#include "str.h"

#define USAGE " cdb cdb.tmp dir"
#define VERSION "$Id: 940f83f1189944815f693d2f54ee1616b3f85763 $"
#define FATAL "ipsvd-cdb: fatal: "
#define WARNING "ipsvd-cdb: warning: "

const char *progname;
char *instdir;
char *cdbfn;
char *tmpfn;

struct cdb_make c;
struct cdb cdb;
int fdcdb;
int fdtmp;
static stralloc sa ={0};
static stralloc tmp ={0};

void usage() { strerr_die4x(111, "usage: ", progname, USAGE, "\n"); }
void die_nomem() { strerr_die2x(111, FATAL, "out of memory."); }
void fatal(char *m0) { strerr_die3sys(111, FATAL, m0, ": "); }
void fatal2(char *m0, char *m1) {
  strerr_die5sys(111, FATAL, m0, ": ", m1, ": ");
}
void warn(char *m0, char *m1) { strerr_warn4(WARNING, m0, ": ", m1, 0); }

int main(int argc, char **argv) {
  int mydir;
  DIR *dir;
  direntry *d;
  struct stat s;
  int ac;
  int i;

  progname =*argv++;

  if (! argv || ! *argv) usage();
  cdbfn =*argv++;
  if (! argv || ! *argv) usage();
  tmpfn =*argv++;
  if (! argv || ! *argv) usage();
  instdir =*argv;

  if ((mydir =open_read(".")) == -1)
    fatal("unable to open current directory");

  /* open cdb.tmp */
  if ((fdtmp =open_trunc(tmpfn)) == -1) fatal2("unable to create", tmpfn);
  if (cdb_make_start(&c, fdtmp) == -1) fatal2("unable to create", tmpfn);

  if (chdir(instdir) == -1) fatal2("unable to change dir", instdir);
  if (! (dir =opendir("."))) fatal2("unable to open dir", instdir);
  errno =0;
  while ((d =readdir(dir))) {
    if (d->d_name[0] == '.') {
      if (d->d_name[1] == 0) continue;
      if (d->d_name[1] == '.') continue;
    }
    if (stat(d->d_name, &s) == -1) {
      warn("unable to stat", d->d_name);
      errno =0;
      continue;
    }
    sa.len =0;
    ac =ipsvd_check(0, &sa, &tmp, ".", 0, d->d_name, 0);
    if (ac == -1) fatal2("unable to read instruction", d->d_name);
    if (ac == IPSVD_ERR) fatal2("unable to read", "."); /* impossible? */

    switch(ac) {
    case IPSVD_EXEC:
      sa.s[sa.len -1] ='X';
      if (cdb_make_add(&c, d->d_name, str_len(d->d_name), sa.s, sa.len) == -1)
        fatal2("unable to add entry", instdir);
      continue;
    case IPSVD_DENY:
      if (! sa.len) {
        if (cdb_make_add(&c, d->d_name, str_len(d->d_name), "D", 1) == -1)
          fatal2("unable to add entry", instdir);
        continue;
      }
    case IPSVD_INSTRUCT:
      for (i =0; i < sa.len; i++) if (sa.s[i] == '\n') sa.s[i] =0;
      sa.s[sa.len -1] ='I';
      if (cdb_make_add(&c, d->d_name, str_len(d->d_name), sa.s, sa.len) == -1)
        fatal2("unable to add entry", instdir);
      continue;
    }
    warn("ignore", d->d_name);
  }
  /* catch errno? */
  closedir(dir);
  if (cdb_make_finish(&c) == -1) fatal2("unable to write cdb", tmpfn);
  if (fsync(fdtmp) == -1) fatal2("unable to write cdb", tmpfn);
  close(fdtmp);
  if (fchdir(mydir) == -1) fatal("unable to change to previous directory");
  close(mydir);
  if (rename(tmpfn, cdbfn) == -1) fatal2("unable to replace", cdbfn);
  _exit(0);
}
