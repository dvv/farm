#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "dns.h"
#include "socket.h"
#include "ip4.h"
#include "ipsvd_check.h"
#include "ipsvd_log.h"
#include "ipsvd_fmt.h"
#include "ipsvd_hostname.h"
#include "ipsvd_phcc.h"
#include "ipsvd_scan.h"
#include "uidgid.h"
#include "str.h"
#include "byte.h"
#include "error.h"
#include "strerr.h"
#include "sgetopt.h"
#include "scan.h"
#include "fmt.h"
#include "sig.h"
#include "fd.h"
#include "wait.h"
#include "prot.h"
#include "pathexec.h"
#include "ndelay.h"

#ifdef SSLSVD
#include "matrixSsl.h"
#include "ssl_io.h"
#endif

#ifdef SSLSVD
#define USAGE " [-Ehpv] [-u user] [-c n] [-C n:msg] [-b n] [-l name] [-i dir|-x cdb] [-t sec] [-U ssluser] [-/ root] [-Z cert] [-K key] host port prog"
#else
#define USAGE " [-Ehpv] [-u user] [-c n] [-C n:msg] [-b n] [-l name] [-i dir|-x cdb] [-t sec] host port prog"
#endif

#define VERSION "$Id: 4f21b1c1026b4c301191b8ee0bc43f3f9090d52c $"

#define FATAL "tcpsvd: fatal: "
#define WARNING "tcpsvd: warning: "
#define INFO "tcpsvd: info: "
#define DROP "tcpsvd: drop: "

char *progname;

unsigned int lookuphost =0;
unsigned int verbose =0;
unsigned long backlog =20;
unsigned int paranoid =0;
const char **prog;
unsigned long cnum =0;
unsigned long cmax =30;
unsigned long timeout =0;

unsigned int ucspi =1;
const char *instructs =0;
unsigned int iscdb =0;
static stralloc local_hostname ={0};
char local_ip[IP4_FMT];
char *local_port;
static stralloc remote_hostname ={0};
char remote_ip[IP4_FMT];
char remote_port[FMT_ULONG];
#ifdef SSLSVD
#else
struct uidgid ugid;
#endif

static char seed[128];
char bufnum[FMT_ULONG];
struct sockaddr_in socka;
int socka_size;
unsigned int phcc =0;

static stralloc sa ={0};
static stralloc ips ={0};
static stralloc fqdn ={0};
static stralloc inst ={0};
static stralloc match ={0};
static stralloc msg ={0};

void usage() { strerr_die4x(111, "usage: ", progname, USAGE, "\n"); }
void die_nomem() { strerr_die2x(111, FATAL, "out of memory."); }
void fatal(char *m0) { strerr_die3sys(111, FATAL, m0, ": "); }
void fatal2(char *m0, char *m1) {
  strerr_die5sys(111, FATAL, m0, ": ", m1, ": ");
}
void warn(char *m0) { strerr_warn3(WARNING, m0, ": ", &strerr_sys); }
void warn2(char *m0, char *m1) {
  strerr_warn5(WARNING, m0, ": ", m1, ": ", &strerr_sys);
}
void drop_nomem() { strerr_die2x(111, DROP, "out of memory."); }
void drop(char *m0) { strerr_die3sys(111, DROP, m0, ": "); }
void drop2(char *m0, char *m1) {
  strerr_die5sys(111, DROP, m0, ": ", m1, ": ");
}

void ucspi_env() {
  char *l =local_hostname.s;
  char *r =remote_hostname.s;

  /* setup ucspi env */
  if (! pathexec_env("PROTO", "TCP")) drop_nomem();
  if (! pathexec_env("TCPLOCALIP", local_ip)) drop_nomem();
  if (! pathexec_env("TCPLOCALPORT", local_port)) drop_nomem();
  if (! pathexec_env("TCPLOCALHOST", *l ? l : 0)) drop_nomem();
  if (! pathexec_env("TCPREMOTEIP", remote_ip)) drop_nomem();
  if (! pathexec_env("TCPREMOTEPORT", remote_port)) drop_nomem();
  if (! pathexec_env("TCPREMOTEHOST", *r ? r : 0)) drop_nomem();
  if (! pathexec_env("TCPREMOTEINFO", 0)) drop_nomem();
  /* additional */
  if (phcc > 0) {
    bufnum[fmt_ulong(bufnum, phcc)] =0;
    if (! pathexec_env("TCPCONCURRENCY", bufnum)) drop_nomem();
  }
}

void connection_status() {
  bufnum[fmt_ulong(bufnum, cnum)] =0;
  out(INFO); out("status "); out(bufnum); out("/");
  bufnum[fmt_ulong(bufnum, cmax)] =0;
  out(bufnum); flush("\n");
}

void sig_term_handler() {
  if (phccmax) ipsvd_phcc_free();
  if (verbose) {
    out(INFO); flush("sigterm received, exit.\n");
  }
  _exit(0);
}
void sig_child_handler() {
  int wstat;
  int i;

  while ((i =wait_nohang(&wstat)) > 0) {
    if (phccmax) ipsvd_phcc_rem(i);
    if (cnum) cnum--;
    if (verbose) {
      bufnum[fmt_ulong(bufnum, i)] =0;
      out(INFO); out("end "); out(bufnum); out(" exit ");
      bufnum[fmt_ulong(bufnum, (unsigned long)wait_exitcode(wstat))] =0;
      out(bufnum); flush("\n");
    }
  }
  if (verbose) connection_status();
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
  socka_size =sizeof(socka);
  if (getsockname(c, (struct sockaddr*)&socka, &socka_size) == -1)
    drop("unable to get local address");
  if (! local_hostname.len) {
    if (dns_name4(&local_hostname, (char*)&socka.sin_addr) == -1)
      drop("unable to look up local hostname");
    if (! stralloc_0(&local_hostname)) die_nomem();
  }
  local_ip[ipsvd_fmt_ip(local_ip, (char*)&socka.sin_addr)] =0;

  if (ucspi) ucspi_env();
  if (instructs) {
    ac =ipsvd_check(iscdb, &inst, &match, (char*)instructs,
                    remote_ip, remote_hostname.s, timeout);
    if (ac == -1) drop2("unable to check inst", remote_ip);
    if (ac == IPSVD_ERR) drop2("unable to read", (char*)instructs);
  }
  else ac =IPSVD_DEFAULT;

  if (phccmax) {
    if (phcc > phccmax) {
      ac =IPSVD_DENY;
      if (phccmsg) {
        ndelay_on(c);
        if (write(c, phccmsg, str_len(phccmsg)) == -1)
          warn("unable to write concurrency message");
      }
    }
    if (verbose) {
      bufnum[fmt_ulong(bufnum, getpid())] =0;
      out(INFO); out("concurrency "); out(bufnum); out(" ");
      outfix(remote_ip); out(" ");
      bufnum[fmt_ulong(bufnum, phcc)] =0;
      out(bufnum); out("/");
      bufnum[fmt_ulong(bufnum, phccmax)] =0;
      out(bufnum); out("\n");
    }
  }
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
    close(c);
    _exit(100);
  }
  if (ac == IPSVD_EXEC) {
    args[0] ="/bin/sh"; args[1] ="-c"; args[2] =inst.s; args[3] =0;
    run =args;
  }
  else run =prog;
  if ((fd_move(0, c) == -1) || (fd_copy(1, 0) == -1))
    drop("unable to set filedescriptor");
  sig_uncatch(sig_term);
  sig_uncatch(sig_pipe);
  sig_uncatch(sig_child);
  sig_unblock(sig_child);
#ifdef SSLSVD
  pid =getpid();
  id[fmt_ulong(id, pid)] =0;
  ssl_io(0, run);
#else
  pathexec(run);
#endif

  drop2("unable to run", (char *)*prog);
}

int main(int argc, char **argv) {
  int opt;
  char *user =0;
  char *host;
  unsigned long port;
  int pid;
  int s;
  int conn;
  int delim;

  progname =*argv;
  phccmax =0;

#ifdef SSLSVD
  while ((opt =getopt(argc, (const char **)argv,
                      "c:C:i:x:u:l:Eb:hpt:vVU:/:Z:K:")) != opteof) {
#else
  while ((opt =getopt(argc, (const char **)argv,
                      "c:C:i:x:u:l:Eb:hpt:vV")) != opteof) {
#endif
    switch(opt) {
    case 'c': scan_ulong(optarg, &cmax); if (cmax < 1) usage(); break;
    case 'C':
      delim =scan_ulong(optarg, &phccmax);
      if (phccmax < 1) usage();
      if (optarg[delim] == ':') {
        if (ipsvd_fmt_msg(&msg, optarg +delim +1) == -1) die_nomem();
        if (! stralloc_0(&msg)) die_nomem();
        phccmsg =msg.s;
      }
      break;
    case 'i': if (instructs) usage(); instructs =optarg; break;
    case 'x': if (instructs) usage(); instructs =optarg; iscdb =1; break;
    case 'u': user =(char*)optarg; break;
    case 'l':
      if (! stralloc_copys(&local_hostname, optarg)) die_nomem();
      if (! stralloc_0(&local_hostname)) die_nomem();
      break;
    case 'E': ucspi =0; break;
    case 'b': scan_ulong(optarg, &backlog); break;
    case 'h': lookuphost =1; break;
    case 'p': lookuphost =1; paranoid =1; break;
    case 't': scan_ulong(optarg, &timeout); break;
    case 'v': ++verbose; break;
#ifdef SSLSVD
    case 'U': ssluser =(char*)optarg; break;
    case '/': root =(char*)optarg; break;
    case 'Z': cert =(char*)optarg; break;
    case 'K': key =(char*)optarg; break;
#endif
    case 'V': strerr_warn1(VERSION, 0);
    case '?': usage();
    }
  }
  argv +=optind;

  if (! argv || ! *argv) usage();
  host =*argv++;
  if (! argv || ! *argv) usage();
  local_port =*argv++;
  if (! argv || ! *argv) usage();
  prog =(const char **)argv;
  if (phccmax > cmax) phccmax =cmax;

  if (user)
    if (! uidgids_get(&ugid, user)) {
      if (errno)
        strerr_die4sys(111, FATAL, "unable to get user/group: ", user, ": ");
      strerr_die3x(100, FATAL, "unknown user/group: ", user);
    }
#ifdef SSLSVD
  svuser =user;
  client =0;
  if ((getuid() == 0) && (! ssluser))
    strerr_die2x(100, FATAL, "-U ssluser must be set when running as root");
  if (ssluser)
    if (! uidgids_get(&sslugid, ssluser)) {
      if (errno)
        strerr_die4sys(111, FATAL, "unable to get user/group: ", ssluser, ": ");
      strerr_die3x(100, FATAL, "unknown user/group: ", ssluser);
    }
  if (! cert) cert ="./cert.pem";
  if (! key) key =cert;
  if (matrixSslOpen() < 0) fatal("unable to initialize ssl");
  if (matrixSslReadKeys(&keys, cert, key, 0, ca) < 0) {
    if (client) fatal("unable to read cert, key, or ca file");
    fatal("unable to read cert or key file");
  }
  if (matrixSslNewSession(&ssl, keys, 0, SSL_FLAGS_SERVER) < 0)
    strerr_die2x(111, FATAL, "unable to create ssl session");
#endif

  dns_random_init(seed);
  sig_block(sig_child);
  sig_catch(sig_child, sig_child_handler);
  sig_catch(sig_term, sig_term_handler);
  sig_ignore(sig_pipe);

  if (phccmax) if (ipsvd_phcc_init(cmax) == -1) die_nomem();

  if (str_equal(host, "")) host ="0.0.0.0";
  if (str_equal(host, "0")) host ="0.0.0.0";

  if (! ipsvd_scan_port(local_port, "tcp", &port))
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

  if (! lookuphost) {
    if (! stralloc_copys(&remote_hostname, "")) die_nomem();
    if (! stralloc_0(&remote_hostname)) die_nomem();
  }

  if ((s =socket_tcp()) == -1) fatal("unable to create socket");
  if (socket_bind4_reuse(s, ips.s, port) == -1)
    fatal("unable to bind socket");
  if (listen(s, backlog) == -1) fatal("unable to listen");
  ndelay_off(s);

#ifdef SSLSVD
#else
  if (user) {
    /* drop permissions */
    if (setgroups(ugid.gids, ugid.gid) == -1) fatal("unable to set groups");
    if (setgid(*ugid.gid) == -1) fatal("unable to set gid");
    if (prot_uid(ugid.uid) == -1) fatal("unable to set uid");
  }
#endif
  close(0);

  if (verbose) {
    out(INFO); out("listening on "); outfix(local_ip); out(":");
    outfix(local_port);
#ifdef SSLSVD
#else
    if (user) {
      bufnum[fmt_ulong(bufnum, (unsigned long)ugid.uid)] =0;
      out(", uid "); out(bufnum);
      bufnum[fmt_ulong(bufnum, (unsigned long)ugid.gid)] =0;
      out(", gid "); out(bufnum);
    }
#endif
    flush(", starting.\n");
  }
  for (;;) {
    while (cnum >= cmax) sig_pause();
    socka_size =sizeof(socka);

    sig_unblock(sig_child);
    conn =accept(s, (struct sockaddr *)&socka, &socka_size);
    sig_block(sig_child);

    if (conn == -1) {
      if (errno != error_intr) warn("unable to accept connection");
      continue;
    }
    cnum++;

    if (verbose) connection_status();
    if (phccmax) phcc =ipsvd_phcc_add((char*)&socka.sin_addr);
    if ((pid =fork()) == -1) {
      warn2("drop connection", "unable to fork");
      close(conn);
      continue;
    }
    if (pid == 0) {
      /* child */
      close(s);
#ifdef SSLSVD
      if (*progname) *progname ='\\';
#endif
      connection_accept(conn);
    }
    if (phccmax) ipsvd_phcc_setpid(pid);
    close(conn);
  }
  _exit(0);
}
