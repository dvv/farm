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

#define USAGEROOT " -u user [-U user] [-/ root] [-C cert] [-K key] [-A ca] [-vc] prog"
#define USAGE " [-C cert] [-K key] [-A ca] [-cv] prog"
#define VERSION "$Id: 5b49a0244b79fd00a69b028b11d60dde84c7a8a5 $"
#define NAME "sslio["
#define FATAL "]: fatal: "
#define WARNING "]: warning: "
#define INFO "]: info: "

const char *progname;
extern char id[FMT_ULONG];
extern char ul[FMT_ULONG];
extern int pid;

static void finish(void);
static void die_nomem() {
  strerr_die4x(111, NAME, id, FATAL, "out of memory.");
}
static void fatalm(char *m0) { strerr_die5sys(111, NAME, id, FATAL, m0, ": "); }
static void fatalmx(char *m0) { strerr_die4x(111, NAME, id, FATAL, m0); }
static void fatal(char *m0) {
  strerr_warn5(NAME, id, FATAL, m0, ": ", &strerr_sys); finish();
  _exit(111);
}
static void fatalx(char *m0) {
  strerr_warn4(NAME, id, FATAL, m0, 0); finish();
  _exit(111);
}
static void fatals(char *m0, int e) {
  strerr_warn6(NAME, id, FATAL, m0, ": ", sslerror_str(e), 0); finish();
  _exit(111);
}
static void warn(char *m0) {
  strerr_warn5(NAME, id, WARNING, m0, ": ", &strerr_sys);
}
static void warnx(char *m0) { strerr_warn4(NAME, id, WARNING, m0, 0); }
static void info(char *m0) { strerr_warn4(NAME, id, INFO, m0, 0); }
static void infou(char *m0, unsigned long u) {
  ul[fmt_ulong(ul, u)] =0;
  strerr_warn5(NAME, id, INFO, m0, ul, 0);
}

extern char *cert;
extern char *key;
extern char *ssluser;
extern char *ca;
extern char *svuser;
extern char *root;
extern unsigned int client;
extern unsigned int verbose;
extern struct uidgid ugid, sslugid;
extern ssl_t *ssl;
extern sslKeys_t *keys;

static unsigned long bufsizein =8192;
static unsigned long bufsizeou =12288;

static int encpipe[2];
static int decpipe[2];
static int len;
static int rc;
static int handshake =1;
static int getdec =1;
static char *s;
static unsigned long handshake_timeout =300;

static sslBuf_t encin, encou;
static stralloc encinbuf ={0};
static stralloc encoubuf ={0};
static sslBuf_t decin, decou;
static stralloc decinbuf ={0};
static stralloc decoubuf ={0};

static int fdstdin =0;
static int fdstdou =1;
static unsigned long bytesin =0;
static unsigned long bytesou =0;

static unsigned char error, alvl, adesc;
static char *bad_certificate;

static void sig_term_handler(void) {
  if (verbose) info("sigterm received, exit.");
  finish();
  _exit(0);
}
unsigned int blowup(sslBuf_t *buf, stralloc *sa, unsigned int len) {
  sa->len =buf->end -buf->buf;
  buf->size +=len;
  if (! stralloc_ready(sa, buf->size)) return(0);
  buf->end =sa->s +(buf->end -buf->buf);
  buf->start =sa->s +(buf->start -buf->buf);
  buf->buf =sa->s;
  return(1);
}
void finish(void) {
  if (fdstdou != -1)
    for (;;) {
      decou.start =decou.end =decou.buf;
      rc =matrixSslEncodeClosureAlert(ssl, &decou);
      if (rc == SSL_FULL) {
        if (! blowup(&decou, &decoubuf, bufsizeou)) die_nomem();
        if (verbose > 1) infou("decode output buffer size: ", decou.size);
        continue;
      }
      if (rc == SSL_ERROR)
        if (verbose) warnx("unable to encode ssl close notify");
      if (rc == 0) {
        if (write(fdstdou, decou.start, decou.end -decou.start)
            != (decou.end -decou.start)) {
          if (verbose) warn("unable to send ssl close notify");
          break;
        }
        if (verbose > 2) info("sending ssl close notify");
        if (verbose > 2) infou("write bytes: ", decou.end -decou.start);
        bytesou +=decou.end -decou.start;
      }
      break;
    }
  /* bummer */
  matrixSslFreeKeys(keys);
  matrixSslDeleteSession(ssl);
  matrixSslClose();
  if (fdstdou != -1) close(fdstdou);
  if (encpipe[0] != -1) close(encpipe[0]);
  if (fdstdin != -1) close(fdstdin);
  if (decpipe[1] != -1) close(decpipe[1]);
  if (verbose) { infou("bytes in: ", bytesin); infou("bytes ou: ", bytesou); }
}
int validate(sslCertInfo_t *cert, void *arg) {
  sslCertInfo_t *c =cert;

  if (bad_certificate) return 1;
  while (c->next) c =c->next;
  return(c->verified);
}

void encode(void) {
  if ((len =read(encpipe[0], encinbuf.s, encin.size)) < 0)
    fatal("unable to read from prog");
  if (len == 0) {
    if (verbose > 2) info("eof reading from proc");
    finish();
    _exit(0);
  }
  for (;;) {
    rc =matrixSslEncode(ssl, encin.buf, len, &encou);
    if (rc == SSL_ERROR) {
      close(fdstdou); fdstdou =-1;
      fatalx("unable to encode data");
    }
    if (rc == SSL_FULL) {
      if (! blowup(&encou, &encoubuf, bufsizeou)) die_nomem();
      if (verbose > 1) infou("encode output buffer size: ", encou.size);
      continue;
    }
    if (write(fdstdou, encou.start, encou.end -encou.start)
        != encou.end -encou.start) fatal("unable to write to network");
    if (verbose > 2) infou("write bytes: ", encou.end -encou.start);
    bytesou +=encou.end -encou.start;
    encou.start =encou.end =encou.buf =encoubuf.s;
    return;
  }
}
void decode(void) {
  do {
    if (getdec) {
      len =decin.size -(decin.end -decin.buf);
      if ((len =read(fdstdin, decin.end, len)) < 0)
        fatal("unable to read from network");
      if (len == 0) {
        if (verbose > 2) info("eof reading from network");
        close(fdstdin); close(decpipe[1]);
        fdstdin =decpipe[1] =-1;
        return;
      }
      if (verbose > 2) infou("read bytes: ", len);
      bytesin +=len;
      decin.end +=len;
      getdec =0;
    }
    for (;;) {
      rc =matrixSslDecode(ssl, &decin, &decou, &error, &alvl, &adesc);
      if (rc == SSL_SUCCESS) break;
      if (rc == SSL_ERROR) {
        if (decou.end > decou.start)
          if (write(fdstdou, decou.start, decou.end -decou.start)
              != decou.end -decou.start) warn("unable to write to network");
        close(fdstdou); fdstdou =-1;
        fatals("ssl decode error", error);
      }
      if (rc == SSL_PROCESS_DATA) {
        if (write(decpipe[1], decou.start, decou.end -decou.start)
            != decou.end -decou.start) fatal("unable to write to prog");
        decou.start =decou.end =decou.buf;
        if (decin.start > decin.buf) { /* align */
          byte_copy(decin.buf, decin.end -decin.start, decin.start);
          decin.end -=decin.start -decin.buf;
          decin.start =decin.buf;
        }
        break;
      }
      if (rc == SSL_SEND_RESPONSE) {
        if (write(fdstdou, decou.start, decou.end -decou.start)
            != (decou.end -decou.start))
          fatal("unable to send ssl response");
        bytesou +=decou.end -decou.start;
        if (verbose > 2) info("sending ssl handshake response");
        if (verbose > 2) infou("write bytes: ", decou.end -decou.start);
        decou.start =decou.end =decou.buf;
        break;
      }
      if (rc == SSL_ALERT) {
        close(fdstdou); fdstdou =-1;
        if (adesc != SSL_ALERT_CLOSE_NOTIFY)
          fatals("ssl alert from peer", adesc);
        if (verbose > 2) info("ssl close notify from peer");
        finish();
        _exit(0);
      }
      if (rc == SSL_PARTIAL) {
        getdec =1;
        if (decin.size -(decin.end -decin.buf) < bufsizein) {
          if (! blowup(&decin, &decinbuf, bufsizein)) die_nomem();
          if (verbose > 1) infou("decode input buffer size: ", decin.size);
        }
        break;
      }
      if (rc == SSL_FULL) {
        if (! blowup(&decou, &decoubuf, bufsizeou)) die_nomem();
        if (verbose > 1) infou("decode output buffer size: ", decou.size);
        continue;
      }
    }
    if (decin.start == decin.end) {
      decin.start =decin.end =decin.buf;
      getdec =1;
    }
  } while (getdec == 0);
  if (handshake)
    if (matrixSslHandshakeIsComplete(ssl)) {
      handshake =0;
      if (verbose > 2) info("ssl handshake complete");
    }
}

void doio(void) {
  iopause_fd x[2];
  struct taia deadline;
  struct taia now;
  struct taia timeout;

  if (! stralloc_ready(&encinbuf, bufsizein)) die_nomem();
  encin.buf =encin.start =encin.end =encinbuf.s; encin.size =bufsizein;
  if (! stralloc_ready(&decinbuf, bufsizein)) die_nomem();
  decin.buf =decin.start =decin.end =decinbuf.s; decin.size =bufsizein;
  if (! stralloc_ready(&encoubuf, bufsizeou)) die_nomem();
  encou.buf =encou.start =encou.end =encoubuf.s; encou.size =bufsizeou;
  if (! stralloc_ready(&decoubuf, bufsizeou)) die_nomem();
  decou.buf =decou.start =decou.end =decoubuf.s; decou.size =bufsizeou;

  if (client) {
    rc =matrixSslEncodeClientHello(ssl, &decou, 0);
    if (rc != 0) fatalx("unable to encode client hello");
    if (write(fdstdou, decou.start, decou.end -decou.start)
        != (decou.end -decou.start))
      fatal("unable to send client hello");
    if (verbose > 2) info("sending client hello");
    if (verbose > 2) infou("write bytes: ", decou.end -decou.start);
    bytesou +=decou.end -decou.start;
    decou.start =decou.end =decou.buf;
  }

  taia_now(&now);
  taia_uint(&timeout, handshake_timeout);
  taia_add(&timeout, &now, &timeout);

  for (;;) {
    iopause_fd *xx =x;
    int l =2;

    x[0].fd =encpipe[0];
    x[0].events =IOPAUSE_READ;
    x[0].revents =0;
    x[1].fd =fdstdin;
    x[1].events =IOPAUSE_READ;
    x[1].revents =0;

    if ((x[0].fd == -1) || handshake) { --l; ++xx; }
    if (x[1].fd == -1) --l;
    if (! l) return;

    taia_now(&now);
    if (handshake) {
      if (taia_less(&timeout, &now)) {
        if (verbose) info("ssl handshake timeout, exit.");
        return;
      }
      deadline.sec =timeout.sec;
      deadline.nano =timeout.nano;
      deadline.atto =timeout.atto;
    }
    else {
      taia_uint(&deadline, 30);
      taia_add(&deadline, &now, &deadline);
    }
    iopause(xx, l, &deadline, &now);
    
    if (x[0].revents) encode();
    if (x[1].revents) decode();
  }
}

int ssl_io(unsigned int newsession, const char **prog) {
  if (client) { fdstdin =6; fdstdou =7; }
  bad_certificate = env_get("SSLIO_BAD_CERTIFICATE");
  if ((s =env_get("SSLIO_BUFIN"))) scan_ulong(s, &bufsizein);
  if ((s =env_get("SSLIO_BUFOU"))) scan_ulong(s, &bufsizeou);
  if (bufsizein < 64) bufsizein =64;
  if (bufsizeou < 64) bufsizeou =64;
  if ((s =env_get("SSLIO_HANDSHAKE_TIMEOUT")))
    scan_ulong(s, &handshake_timeout);
  if (handshake_timeout < 1) handshake_timeout =1;

  if (pipe(encpipe) == -1) fatalm("unable to create pipe for encoding");
  if (pipe(decpipe) == -1) fatalm("unable to create pipe for decoding");
  if ((pid =fork()) == -1) fatalm("unable to fork");
  if (pid == 0) {
    if (close(encpipe[1]) == -1)
      fatalm("unable to close encoding pipe output");
    if (close(decpipe[0]) == -1)
      fatalm("unable to close decoding pipe input");
    if (newsession) if (matrixSslOpen() < 0) fatalm("unable to initialize ssl");
    if (root) {
      if (chdir(root) == -1) fatalm("unable to change to new root directory");
      if (chroot(".") == -1) fatalm("unable to chroot");
    }
    if (ssluser) {
      /* drop permissions */
      if (setgroups(sslugid.gids, sslugid.gid) == -1)
        fatal("unable to set groups");
      if (setgid(*sslugid.gid) == -1) fatal("unable to set gid");
      if (prot_uid(sslugid.uid) == -1) fatalm("unable to set uid");
    }
    if (newsession) {
      if (matrixSslReadKeys(&keys, cert, key, 0, ca) < 0) {
        if (client) fatalm("unable to read cert, key, or ca file");
        fatalm("unable to read cert or key file");
      }
      if (matrixSslNewSession(&ssl, keys, 0, client?0:SSL_FLAGS_SERVER) < 0)
        fatalmx("unable to create ssl session");
    }
    if (client)
      if (ca || bad_certificate) matrixSslSetCertValidator(ssl, &validate, 0);

    sig_catch(sig_term, sig_term_handler);
    sig_ignore(sig_pipe);
    doio();
    finish();
    _exit(0);
  }
  if (close(encpipe[0]) == -1) fatalm("unable to close encoding pipe input");
  if (close(decpipe[1]) == -1) fatalm("unable to close decoding pipe output");
  if (fd_move(fdstdin, decpipe[0]) == -1)
    fatalm("unable to setup filedescriptor for decoding");
  if (fd_move(fdstdou, encpipe[1]) == -1)
    fatalm("unable to setup filedescriptor for encoding");
  sslCloseOsdep();
  if (svuser) {
    if (setgroups(ugid.gids, ugid.gid) == -1)
      fatal("unable to set groups for prog");
    if (setgid(*ugid.gid) == -1) fatal("unable to set gid for prog");
    if (prot_uid(ugid.uid) == -1) fatalm("unable to set uid for prog");
  }
  pathexec(prog);
  fatalm("unable to run prog");
  return(111);
}
