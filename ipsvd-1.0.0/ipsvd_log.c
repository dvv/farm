#include "buffer.h"
#include "ipsvd_log.h"

void out(char *m) { buffer_puts(buffer_1, m); }
void outfix(char *m) {
  char ch;
  int i;

  if (! m) return;
  for (i =0; i < 100; ++i) {
    ch =m[i];
    if (!ch) return;
    if (ch < 33) ch ='?';
    else if (ch > 126) ch ='?';
    else if (ch == ':') ch ='?';
    buffer_put(buffer_1, &ch, 1);
  }
  out("...(truncate)");
}
void outinst(stralloc *sa) {
  char ch;
  int len, i;

  if (! sa->s || ! sa->len) return;
  for (len =sa->len; len && (sa->s[len -1] == 0); --len);
  for (i =0; i < 140; ++i) {
    if (i >= len) return;
    ch =sa->s[i];
    if (ch == 0) ch =',';
    else if (ch < 32) ch ='?';
    else if (ch > 126) ch ='?';
    buffer_put(buffer_1, &ch, 1);
  }
  out("...(truncate)");
}
void flush(char *m) { buffer_puts(buffer_1, m); buffer_flush(buffer_1); }
