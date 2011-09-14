#include "alloc.h"
#include "byte.h"

struct hcc {
  char ip[4];
  int pid;
};
static struct hcc *cc;
static unsigned int cclen;
static int pos =-1;

unsigned int i;

/* to be optimized */

int ipsvd_phcc_init(unsigned int c) {
  if (cc) alloc_free(cc);
  cc =(struct hcc*)alloc(c *sizeof(struct hcc));
  if (! cc) return(-1);
  for (i =0; i < c; ++i) {
    cc[i].ip[0] =0; cc[i].pid =0;
  }
  cclen =c;
  return(0);
}

unsigned int ipsvd_phcc_add(char *ip) {
  unsigned int rc =1;
  int p =-1;

  for (i =0; i < cclen; ++i) {
    if (cc[i].ip[0] == 0) {
      if (p == -1) p =i;
      continue;
    }
    if (byte_equal(cc[i].ip, 4, ip)) {
      ++rc;
      continue;
    }
  }
  if ((pos =p) == -1) return(0); /* impossible */
  byte_copy(cc[pos].ip, 4, ip);
  return(rc);
}

unsigned int ipsvd_phcc_setpid(int pid) {
  if (pos == -1) return(0); /* should never happen */
  cc[pos].pid =pid;
  return(1);
}

int ipsvd_phcc_rem(int pid) {
  for (i =0; i < cclen; ++i)
    if (cc[i].pid == pid) {
      cc[i].ip[0] =0; cc[i].pid =0;
      return(0);
    }
  return(-1);
}

void ipsvd_phcc_free(void) {
  if (cc) alloc_free(cc);
}
