#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "dns.h"
#include "socket.h"
#include "ip4.h"
#include "ipsvd_log.h"
#include "ipsvd_check.h"
#include "ipsvd_fmt.h"
#include "ipsvd_hostname.h"
#include "ipsvd_scan.h"
#include "uidgid.h"
#include "sgetopt.h"
#include "sig.h"
#include "str.h"
#include "fmt.h"
#include "error.h"
#include "strerr.h"
#include "prot.h"
#include "ndelay.h"
#include "scan.h"
#include "fd.h"
#include "wait.h"
#include "pathexec.h"

#define USAGE " [-hpv] [-u user] [-l name] [-i dir|-x cdb] [-t sec] host port prog"
#define VERSION "$Id: 4991b38bba93e925f0772eaf9b0b96f5788b009a $"

#define FATAL "udpsvd: fatal: "
#define WARNING "udpsvd: warning: "
#define INFO "udpsvd: info: "
#define DROP "udpsvd: drop: "

const char *progname;

const char **prog;
const char *instructs =0;
unsigned int iscdb =0;
unsigned int verbose =0;
unsigned int lookuphost =0;
unsigned int paranoid =0;
unsigned long timeout =0;

static stralloc local_hostname ={0};
char local_ip[IP4_FMT];
char *local_port;
static stralloc remote_hostname ={0};
char remote_ip[IP4_FMT];
char remote_port[FMT_ULONG];
struct uidgid ugid;

static char seed[128];
struct sockaddr_in socka;
int socka_size =sizeof(socka);
char bufnum[FMT_ULONG];
int s;

static stralloc sa ={0};
static stralloc ips ={0};
static stralloc fqdn ={0};
static stralloc inst ={0};
static stralloc match ={0};

void usage() { strerr_die4x(111, "usage: ", progname, USAGE, "\n"); }
void die_nomem() { strerr_die2x(111, FATAL, "out of memory."); }
void fatal(char *m0) { strerr_die3sys(111, FATAL, m0, ": "); };
void fatal2(char *m0, char *m1) {
  strerr_die5sys(111, FATAL, m0, ": ", m1, ": ");
}
void warn(char *m0) { strerr_warn3(WARNING, m0, ": ", &strerr_sys); }
void warn2(char *m0, char *m1) {
  strerr_warn5(WARNING, m0, ": ", m1, ": ", &strerr_sys);
}
void drop_nomem() { strerr_die2x(111, DROP, "out of memory."); }
void drop(char *m0) { strerr_die3sys(111, DROP, m0, ": "); }
void discard(char *m0, char *m1) {
  recv(s, 0, 0, 0);
  strerr_die6sys(111, DROP, "discard data: ", m0, ": ", m1, ": ");
}

void sig_term_handler() {
  if (verbose) {
    out(INFO); flush("sigterm received, exit.\n");
  }
  _exit(0);
}

void connection_accept(int c) {
  int ac;
  const char **run;
  const char *args[4];
  char *ip =(char*)&socka.sin_addr;

  remote_ip[ipsvd_fmt_ip(remote_ip, ip)] =0;
  if (verbose) {
    out(INFO); out("pid ");
    bufnum[fmt_ulong(bufnum, getpid())] =0;
    out(bufnum); out(" from "); outfix(remote_ip); flush("\n");
  }
  remote_port[ipsvd_fmt_port(remote_port, (char*)&socka.sin_port)] =0;
  if (lookuphost) {
    if (ipsvd_hostname(&remote_hostname, ip, paranoid) == -1)
      warn2("unable to look up hostname", remote_ip);
    if (! stralloc_0(&remote_hostname)) drop_nomem();
  }

  if (instructs) {
    ac =ipsvd_check(iscdb, &inst, &match, (char*)instructs,
                    remote_ip, remote_hostname.s, timeout);
    if (ac == -1) discard("unable to check inst", remote_ip);
    if (ac == IPSVD_ERR) discard("unable to read", (char*)instructs);
  }
  else ac =IPSVD_DEFAULT;

  if (verbose) {
    out(INFO);
    switch(ac) {
    case IPSVD_DENY: out("deny "); break;
    case IPSVD_DEFAULT: case IPSVD_INSTRUCT: out("start "); break;
    case IPSVD_EXEC: out("exec "); break;
    }
    bufnum[fmt_ulong(bufnum, getpid())] =0;
    out(bufnum); out(" ");
    outfix(local_hostname.s); out(":"); out(local_ip);
    out(" :"); outfix(remote_hostname.s); out(":");
    outfix(remote_ip); out(":"); outfix(remote_port);
    if (instructs) {
      out(" ");
      if (iscdb) {
        out((char*)instructs); out("/");
      }
      outfix(match.s);
      if(inst.s && inst.len && (verbose > 1)) {
        out(": "); outinst(&inst);
      }
    }
    flush("\n");
  }

  if (ac == IPSVD_DENY) {
    recv(s, 0, 0, 0);
    _exit(100);
  }
  if (ac == IPSVD_EXEC) {
    args[0] ="/bin/sh"; args[1] ="-c"; args[2] =inst.s; args[3] =0;
    run =args;
  }
  else run =prog;

  if ((fd_move(0, c) == -1) || (fd_copy(1, 2) == -1))
    drop("unable to set filedescriptor");
  sig_uncatch(sig_term);
  sig_uncatch(sig_pipe);
  pathexec(run);

  discard("unable to run", (char*)*prog);
}

int main(int argc, const char **argv, const char *const *envp) {
  int opt;
  char *user =0;
  char *host;
  unsigned long port;
  int pid;
  int wstat;
  iopause_fd io[1];
  struct taia now;
  struct taia deadline;

  progname =*argv;

  while ((opt =getopt(argc, argv, "vu:l:hpi:x:t:V")) != opteof) {
    switch(opt) {
    case 'v':
      ++verbose;
      break;
    case 'u':
      user =(char*)optarg;
      break;
    case 'l':
      if (! stralloc_copys(&local_hostname, optarg)) die_nomem();
      if (! stralloc_0(&local_hostname)) die_nomem();
      break;
    case 'h':
      lookuphost =1;
      break;
    case 'p':
      lookuphost =1;
      paranoid =1;
      break;
    case 'i':
      if (instructs) usage();
      instructs =optarg;
      break;
    case 'x':
      if (instructs) usage();
      instructs =optarg;
      iscdb =1;
      break;
    case 't':
      scan_ulong(optarg, &timeout);
      break;
    case 'V':
      strerr_warn1(VERSION, 0);
    case '?':
      usage();
    }
  }
  argv +=optind;

  if (! argv || ! *argv) usage();
  host =(char*)*argv++;
  if (! argv || ! *argv) usage();
  local_port =(char*)*argv++;
  if (! argv || ! *argv) usage();
  prog =argv;

  if (user)
    if (! uidgids_get(&ugid, user))
      strerr_die3x(100, FATAL, "unknown user/group: ", user);

  dns_random_init(seed);
  sig_catch(sig_term, sig_term_handler);
  sig_ignore(sig_pipe);

  if (str_equal(host, "")) host ="0.0.0.0";
  if (str_equal(host, "0")) host ="0.0.0.0";

  if (! ipsvd_scan_port(local_port, "udp", &port))
    strerr_die3x(100, FATAL, "unknown port number or name: ", local_port);

  if (! stralloc_copys(&sa, host)) die_nomem();
  if ((dns_ip4(&ips, &sa) == -1) || (ips.len < 4))
    if (dns_ip4_qualify(&ips, &fqdn, &sa) == -1)
      fatal2("unable to look up ip address", host);
  if (ips.len < 4)
    strerr_die3x(100, FATAL, "unable to look up ip address: ", host);
  ips.len =4;
  if (! stralloc_0(&ips)) die_nomem();
  local_ip[ipsvd_fmt_ip(local_ip, ips.s)] =0;
  if (! local_hostname.len) {
    if (dns_name4(&local_hostname, ips.s) == -1)
      fatal("unable to look up local hostname");
    if (! stralloc_0(&local_hostname)) die_nomem();
  }

  if (! lookuphost) {
    if (! stralloc_copys(&remote_hostname, "")) die_nomem();
    if (! stralloc_0(&remote_hostname)) die_nomem();
  }

  if ((s =socket_udp()) == -1) fatal("unable to create socket");
  if (socket_bind4_reuse(s, ips.s, port) == -1)
    fatal("unable to bind socket");
  ndelay_off(s);

  if (user) { /* drop permissions */
    if (setgroups(ugid.gids, ugid.gid) == -1) fatal("unable to set groups");
    if (setgid(*ugid.gid) == -1) fatal("unable to set gid");
    if (prot_uid(ugid.uid) == -1) fatal("unable to set uid");
  }
  close(0);

  if (verbose) {
    out(INFO); out("listening on "); outfix(local_ip); out(":");
    outfix(local_port);
    if (user) {
      bufnum[fmt_ulong(bufnum, ugid.uid)] =0;
      out(", uid "); out(bufnum);
      bufnum[fmt_ulong(bufnum, ugid.gid)] =0;
      out(", gid "); out(bufnum);
    }
    flush(", starting.\n");
  }

  io[0].fd =s;
  io[0].events =IOPAUSE_READ;
  io[0].revents =0;
  for (;;) {
    taia_now(&now);
    taia_uint(&deadline, 3600);
    taia_add(&deadline, &now, &deadline);
    iopause(io, 1, &deadline, &now);

    if (io[0].revents | IOPAUSE_READ) {
      io[0].revents =0;
      while ((pid =fork()) == -1) {
        warn("unable to fork, sleeping");
        sleep(5);
      }
      if (pid == 0) { /* child */
        if (recvfrom(s, 0, 0, MSG_PEEK, (struct sockaddr *)&socka,
                     &socka_size) == -1) drop("unable to read from socket");
        connection_accept(s);
      }
      while (wait_pid(&wstat, pid) == -1) warn("error waiting for child");
      if (verbose) {
        out(INFO); out("end ");
        bufnum[fmt_ulong(bufnum, pid)] =0;
        out(bufnum); flush("\n");
      }
    }
  }
  _exit(0);
}
