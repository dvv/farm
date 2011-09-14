#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "ipsvd_check.h"
#include "ipsvd_log.h"
#include "ipsvd_fmt.h"
#include "error.h"
#include "stralloc.h"
#include "strerr.h"
#include "byte.h"
#include "scan.h"
#include "str.h"
#include "openreadclose.h"
#include "open.h"
#include "cdb.h"
#include "pathexec.h"
#include "dns.h"
#include "ip4.h"

extern const char *progname;
static stralloc sa ={0};
static stralloc ips ={0};
static stralloc fqdn ={0};
static stralloc msg ={0};
static stralloc forwardfn ={0};
static stralloc moredata ={0};
static char *forward =0;
unsigned long phccmax;
char *phccmsg;

static struct cdb c;
static int fd;
static uint32 dlen;

int ipsvd_instruct(stralloc *inst, stralloc *match, char *ip) {
  char *insts;
  unsigned int instslen;
  int delim;
  int i, j;
  int rc =IPSVD_DEFAULT;

  if (inst->s && inst->len) {
    insts =inst->s; instslen =inst->len;
    while ((i =byte_chr(insts, instslen, 0)) < instslen) {
      switch(*insts) {
      case '+':
        if ((delim =str_chr(insts, '=')) <= 1) break; /* empty inst */
        if (insts[delim] == '=') {
          insts[delim] =0;
          if (! pathexec_env(insts +1, insts +delim +1)) return(-1);
          insts[delim] ='=';
        }
        else if (! pathexec_env(insts +1, 0)) return(-1);
        break;
      case 'C':
        if (! phccmax) break;
        delim =scan_ulong(insts +1, &phccmax);
        if (insts[delim +1] == ':') {
          if (ipsvd_fmt_msg(&msg, insts +delim +2) == -1) return(-1);
          if (! stralloc_0(&msg)) return(-1);
          phccmsg =msg.s;
        }
        break;
      case '=':
        if (ip && (rc != IPSVD_INSTRUCT)) {
          unsigned int next;

          rc =IPSVD_DENY;
          next =str_chr(insts +1, ':'); ++next;
          if ((next == 2) && (insts[1] == '0')) {
            if (! stralloc_copys(&sa, ip)) return(-1);
          }
          else
            if (! stralloc_copyb(&sa, insts +1, next -1)) return(-1);
          if (insts[next] != 0) ++next;

          if ((dns_ip4(&ips, &sa) == -1) || (ips.len < 4))
            if (dns_ip4_qualify(&ips, &fqdn, &sa) == -1) {
              if (! stralloc_0(&sa)) return(-1);
              strerr_warn5(progname, ": warning: ",
                           "unable to look up ip address: ", sa.s,
                           ": ", &strerr_sys);
              break;
            }
          if (ips.len < 4) {
            if (! stralloc_0(&sa)) return(-1);
            strerr_warn4(progname, ": warning: ",
                         "unable to look up ip address: ", sa.s, 0);
            break;
          }
          for (j =0; j +4 <= ips.len; j +=4) {
            char tmp[IP4_FMT];
            
            tmp[ipsvd_fmt_ip(tmp, ips.s +j)] =0;
            if (str_equal(tmp, ip)) {
              inst->len =insts -inst->s +i +1;
              if (insts[next]) {
                forward =insts +next;
                return(IPSVD_FORWARD);
              }
              return(IPSVD_INSTRUCT);
            }
          }
        }
        break;
      case 0: case '#': /* skip empty line and comment */ 
        break;
      default:
        strerr_warn6(progname, ": warning: ",
                     "bad instruction: ", match->s, ": ", insts, 0);
      }
      insts +=i +1;
      instslen -=i +1;
    }
  }
  if (rc == IPSVD_DEFAULT) return(IPSVD_INSTRUCT);
  return(rc);
}

int ipsvd_check_direntry(stralloc *d, stralloc *m, char *ip,
                         time_t now, unsigned long t, int *rc) {
  int i;
  struct stat s;

  if (stat(m->s, &s) != -1) {
    if (t && (s.st_mode & S_IWUSR) && (now >= s.st_atime))
      if ((now -s.st_atime) >= t) {
        if (unlink(m->s) == -1)
          strerr_warn4(progname, ": warning: unable to unlink: ", m->s, ": ",
                       &strerr_sys);
        return(0);
      }
    if (! (s.st_mode & S_IXUSR) && ! (s.st_mode & S_IRUSR)) {
      *rc =IPSVD_DENY; return(1);
    }
    if (s.st_mode & S_IXUSR) {
      if (openreadclose(m->s, d, 256) <= 0) return(-1);
      if (d->len && (d->s[d->len -1] == '\n')) d->len--;
      if (! stralloc_0(d)) return(-1);
      *rc =IPSVD_EXEC;
      return(1);
    }
    if (s.st_mode & S_IRUSR) {
      if (openreadclose(m->s, d, 256) <= 0) return(-1);
      if (d->len && (d->s[d->len -1] == '\n')) d->len--;
      for (i =0; i < d->len; i++) if (d->s[i] == '\n') d->s[i] =0;
      if (! stralloc_0(d)) return(-1);
      if ((*rc =ipsvd_instruct(d, m, ip)) == -1) return(-1);
      return(1);
    }
    if (! stralloc_copys(m, "")) return(-1);
    if (! stralloc_0(m)) return(-1);
    *rc =IPSVD_DEFAULT;
    return(1);
  }
  else if (errno != error_noent) return(-1);
  return(0);
}

int ipsvd_check_dir(stralloc *data, stralloc *match, char *dir,
                    char *ip, char *name, unsigned long timeout) {
  struct stat s;
  int i;
  int rc;
  int ok;
  int base;
  time_t now =0;

  if (stat(dir, &s) == -1) return(IPSVD_ERR);
  if (timeout) now =time((time_t*)0);
  if (! stralloc_copys(match, dir)) return(-1);
  if (! stralloc_cats(match, "/")) return(-1);
  base =match->len;
  /* ip */
  if (ip) {
    if (! stralloc_cats(match, ip)) return(-1);
    if (! stralloc_0(match)) return(-1);
    data->len =0;
    for (;;) {
      ok =ipsvd_check_direntry(data, match, ip, now, timeout, &rc);
      if (ok == -1) return(-1);
      if (ok) {
        if (rc == IPSVD_FORWARD) goto forwarded;
        return(rc);
      }
      if ((i =byte_rchr(match->s, match->len, '.')) == match->len) break;
      if (i <= base) break;
      match->s[i] =0; match->len =i +1;
    }
  }
  /* host */
  if (name) {
    for (;;) {
      if (! *name || (*name == '.')) break;
      match->len =base;
      if (! stralloc_cats(match, name)) return(-1);
      if (! stralloc_0(match)) return(-1);

      ok =ipsvd_check_direntry(data, match, ip, now, timeout, &rc);
      if (ok == -1) return(-1);
      if (ok) {
        if (rc == IPSVD_FORWARD) goto forwarded;
        return(rc);
      }
      if ((i =byte_chr(name, str_len(name), '.')) == str_len(name)) break;
      name +=i +1;
    }
  }
  /* default */
  match->len =base;
  if (! stralloc_cats(match, "0")) return(-1);
  if (! stralloc_0(match)) return(-1);

  ok =ipsvd_check_direntry(data, match, ip, now, timeout, &rc);
  if (ok == -1) return(-1);
  if (ok) {
    if (rc == IPSVD_FORWARD) goto forwarded;
    return(rc);
  }
  if (! stralloc_copys(match, "")) return(-1);
  if (! stralloc_0(match)) return(-1);
  return(IPSVD_DEFAULT);

 forwarded:
  if (! stralloc_copyb(&forwardfn, match->s, base)) return(-1);
  if (! stralloc_cats(&forwardfn, forward)) return(-1);
  if (! stralloc_0(&forwardfn)) return(-1);
  ok =ipsvd_check_direntry(&moredata, &forwardfn, 0, now, timeout, &rc);
  if (ok == -1) return(-1);
  --match->len;
  if (! stralloc_cats(match, ",")) return(-1);
  if (! stralloc_cats(match, forwardfn.s +base)) return(-1);
  if (! stralloc_0(match)) return(-1);
  if (ok) {
    if (rc == IPSVD_EXEC)
      data->len =0;
    else
      data->s[data->len -1] =',';
    if (! stralloc_cat(data, &moredata)) return(-1);
    return(rc);
  }
  strerr_warn4(progname, ": warning: ", match->s, ": not found", 0);
  return(IPSVD_DENY);
}

int ipsvd_check_cdbentry(stralloc *d, stralloc *m, char *ip, int *rc) {
  switch(cdb_find(&c, m->s, m->len -1)) {
    case -1: return(-1);
    case 1:
      dlen =cdb_datalen(&c);
      if (! stralloc_ready(d, dlen)) return(-1);
      if (cdb_read(&c, d->s, dlen, cdb_datapos(&c)) == -1) return(-1);
      if (! dlen) return(-1);
      switch(d->s[dlen -1]) {
      case 'D':
        *rc =IPSVD_DENY;
        return(1);
      case 'X':
        d->s[dlen -1] =0; d->len =dlen;
        *rc =IPSVD_EXEC;
        return(1);
      case 'I':
        d->s[dlen -1] =0; d->len =dlen;
        if ((*rc =ipsvd_instruct(d, m, ip)) == -1) return(-1);
        return(1);
      }
    }
  return(0);
}

int ipsvd_check_cdb(stralloc *data, stralloc *match, char *cdb,
                    char *ip, char *name, unsigned long unused) {
  int ok;
  int i;
  int rc;

  if ((fd =open_read(cdb)) == -1) return(IPSVD_ERR);
  cdb_init(&c, fd);
  if (! stralloc_copys(match, ip)) return(-1);
  if (! stralloc_0(match)) return(-1);
  data->len =0;
  /* ip */
  for (;;) {
    ok =ipsvd_check_cdbentry(data, match, ip, &rc);
    if (ok == -1) return(-1);
    if (ok) {
      if (rc == IPSVD_FORWARD) goto forwarded;
      close(fd);
      return(rc);
    }
    if ((i =byte_rchr(match->s, match->len, '.')) == match->len) break;
    match->s[i] =0; match->len =i +1;
  }
  /* host */
  if (name) {
    for (;;) {
      if (! *name || (*name == '.')) break;
      if (! stralloc_copys(match, name)) return(-1);
      if (! stralloc_0(match)) return(-1);
      ok =ipsvd_check_cdbentry(data, match, ip, &rc);
      if (ok == -1) return(-1);
      if (ok) {
        if (rc == IPSVD_FORWARD) goto forwarded;
        close(fd);
        return(rc);
      }
      if ((i =byte_chr(name, str_len(name), '.')) == str_len(name)) break;
      name +=i +1;
    }
  }
  /* default */
  if (! stralloc_copys(match, "0")) return(-1);
  if (! stralloc_0(match)) return(-1);
  ok =ipsvd_check_cdbentry(data, match, ip, &rc);
  if (ok == -1) return(-1);
  if (ok) {
    if (rc == IPSVD_FORWARD) goto forwarded;
    close(fd);
    return(rc);
  }
  if (! stralloc_copys(match, "")) return(-1);
  if (! stralloc_0(match)) return(-1);
  close(fd);
  return(IPSVD_DEFAULT);

 forwarded:
  if (! stralloc_copys(&forwardfn, forward)) return(-1);
  if (! stralloc_0(&forwardfn)) return(-1);
  ok =ipsvd_check_cdbentry(&moredata, &forwardfn, 0, &rc);
  close(fd);
  if (ok == -1) return(-1);
  --match->len;
  if (! stralloc_cats(match, ",")) return(-1);
  if (! stralloc_cats(match, forwardfn.s)) return(-1);
  if (! stralloc_0(match)) return(-1);
  if (ok) {
    if (rc == IPSVD_EXEC)
      data->len =0;
    else
      data->s[data->len -1] =',';
    if (! stralloc_cat(data, &moredata)) return(-1);
    return(rc);
  }
  strerr_warn4(progname, ": warning: ", match->s, ": not found", 0);
  return(IPSVD_DENY);
}

int ipsvd_check(int c, stralloc *data, stralloc *match, char *db,
                char *ip, char *name, unsigned long timeout) {
  if (c)
    return(ipsvd_check_cdb(data, match, db, ip, name, 0));
  else
    return(ipsvd_check_dir(data, match, db, ip, name, timeout));
}
