#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include "scan.h"

unsigned int ipsvd_scan_port(const char *s, const char *proto,
                             unsigned long *p) {
  struct servent *se;

  if (! *s) return(0);
  if ((se =getservbyname(s, proto))) {
    /* what is se->s_port, uint16 or uint32? */
    *p =htons(se->s_port);
    return(1);
  }
  if (s[scan_ulong(s, p)]) return(0);
  return(1);
}
