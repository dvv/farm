#include "fmt.h"
#include "stralloc.h"
#include "str.h"

unsigned int ipsvd_fmt_ip(char *s, char ip[4]) {
  char *p =s;
  int i;

  i =fmt_ulong(p, (unsigned long)(unsigned char)ip[0]);
  if (p) p +=i; if (p) *p++ ='.';
  i =fmt_ulong(p, (unsigned long)(unsigned char)ip[1]);
  if (p) p +=i; if (p) *p++ ='.';
  i =fmt_ulong(p, (unsigned long)(unsigned char)ip[2]);
  if (p) p +=i; if (p) *p++ ='.';
  i =fmt_ulong(p, (unsigned long)(unsigned char)ip[3]);
  if (p) p +=i;
  return(p -s);
}
unsigned int ipsvd_fmt_port(char *s, char port[2]) {
  unsigned short u;

  u =(unsigned char)port[0];
  u <<=8;
  u +=(unsigned char)port[1];
  return(fmt_ulong(s, (unsigned long)u));
}

int ipsvd_fmt_msg(stralloc *sa, const char *msg) {
  const char *p;
  int i;

  if (! msg) return(0);
  if (! stralloc_copys(sa, "")) return(-1);
  for (p =msg; *p; ++p) {
    i =str_chr(p, '\\');
    if (! stralloc_catb(sa, p, i)) return(-1);
    if (*(p +=i) == 0) break;
    switch(*++p) {
    case 0: break;
    case '\\':
      if (! stralloc_append(sa, "\\")) return(-1);
      continue;
    case 'n':
      if (! stralloc_append(sa, "\n")) return(-1);
      continue;
    case 'r':
      if (! stralloc_append(sa, "\r")) return(-1);
      continue;
    default:
      if (! stralloc_catb(sa, p -1, 2)) return(-1);
    }
  }
  return(sa->len);
}
