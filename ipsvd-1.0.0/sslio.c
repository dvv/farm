#include <sys/types.h>
#include <unistd.h>
#include "matrixSsl.h"
#include "uidgid.h"
#include "prot.h"
#include "error.h"
#include "strerr.h"
#include "fd.h"
#include "pathexec.h"
#include "stralloc.h"
#include "byte.h"
#include "taia.h"
#include "iopause.h"
#include "ndelay.h"
#include "fmt.h"
#include "scan.h"
#include "sgetopt.h"
#include "env.h"
#include "sig.h"
#include "sslerror_str.h"
#include "ssl_io.h"

#define USAGEROOT " -u user [-U user] [-/ root] [-C cert] [-K key] [-A ca] [-vc] prog"
#define USAGE " [-C cert] [-K key] [-A ca] [-cv] prog"
#define VERSION "$Id: c08b30d00b9743c5bd24fa59a281a90dd4c742c4 $"

const char *progname;
unsigned int verbose =0;

void usage() {
  if (getuid() == 0) strerr_die4x(111, "usage: ", progname, USAGEROOT, "\n");
  strerr_die4x(111, "usage: ", progname, USAGE, "\n");
}

int main(int argc, const char **argv) {
  int opt;

  progname =*argv;
  pid =getpid();
  id[fmt_ulong(id, pid)] =0;

  while ((opt =getopt(argc, argv, "u:U:/:C:K:A:cvV")) != opteof) {
    switch(opt) {
    case 'u': ssluser =(char*)optarg; break;
    case 'U': svuser =(char*)optarg; break;
    case '/': root =(char*)optarg; break;
    case 'C': cert =(char*)optarg; break;
    case 'K': key =(char*)optarg; break;
    case 'c': client =1; break;
    case 'A': ca =(char*)optarg; break;
    case 'v': ++verbose; break;
    case 'V': strerr_warn1(VERSION, 0);
    case '?': usage();
    }
  }
  argv +=optind;
  if (! argv || ! *argv) usage();

  if (getuid() == 0) { if (! ssluser) usage(); }
  else { if (root || ssluser || svuser) usage(); }

  if (! client) {
    if (! cert) cert ="./cert.pem";
    if (! key) key =cert;
  }
  if (ssluser) if (! uidgids_get(&sslugid, ssluser)) {
    if (errno)
      strerr_die3sys(111, "sslio[", id, "]: fatal: unable to get user/group: ");
    strerr_die4x(100, "sslio[", id, "]: fatal: unknown user/group: ", ssluser);
  }
  if (svuser) if (! uidgids_get(&ugid, svuser)) {
    if (errno)
      strerr_die3sys(111, "sslio[", id, "]: fatal: unable to get user/group: ");
    strerr_die4x(100, "sslio[", id, "]: fatal: unknown user/group: ", svuser);
  }

  return(ssl_io(1, argv));
}
