#ifndef IPSVD_FMT_H
#define IPSVD_FMT_H

extern unsigned int ipsvd_fmt_ip(char *s, char ip[4]);
extern unsigned int ipsvd_fmt_port(char *s, char port[2]);

extern int ipsvd_fmt_msg(stralloc *sa, const char *msg);

#endif
