#ifndef IPSVD_LOG_H
#define IPSVD_LOG_H

#include "stralloc.h"

extern void out(char *);
extern void outfix(char *);
extern void outinst(stralloc *);
extern void flush(char *);

#endif
