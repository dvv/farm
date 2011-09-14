#ifndef SSL_IO_H
#define SSL_IO_H
#include "matrixSsl.h"
#include "uidgid.h"

char id[FMT_ULONG];
char ul[FMT_ULONG];

char *cert =0;
char *key =0;
char *ssluser =0;
char *ca =0;
char *svuser =0;
char *root =0;
unsigned int client =0;
int pid;
struct uidgid ugid;
struct uidgid sslugid;
ssl_t *ssl;
sslKeys_t *keys;

int ssl_io(int, const char **);

#endif
