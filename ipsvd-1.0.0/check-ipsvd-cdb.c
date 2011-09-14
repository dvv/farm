#include "buffer.h"
#include "strerr.h"
#include "uint32.h"

int i;
int pos =0;
uint32 eod;
uint32 klen;
uint32 dlen;
char buf[4];
char e[256];

void get(char *buf, unsigned int l) {
  while (l > 0) {
    i =buffer_get(buffer_0, buf, l);
    if (i == -1) strerr_die1sys(111, "fatal: unable to read cdb: ");
    if (i == 0) strerr_die1x(111, "fatal: unable to read cdb: truncated");
    buf +=i; l -=i;
  }
}

int main() {
  get(buf, 4); pos +=4; uint32_unpack(buf, &eod);
  while (pos < 2048) { get(buf, 4); pos +=4; }
  while (pos < eod) {
    get(buf, 4); pos +=4; uint32_unpack(buf, &klen);
    get(buf, 4); pos +=4; uint32_unpack(buf, &dlen);
    if ((klen > 256) || (dlen > 256))
      strerr_die1x(111, "fatal: unable to read cdb: entry too long");
    get(e, klen);
    buffer_put(buffer_1, e, klen); buffer_put(buffer_1, ":", 1);
    get(e, dlen);
    buffer_put(buffer_1, e, dlen); buffer_putflush(buffer_1, "\n", 1);
    pos +=klen +dlen;
  }
  return(0);
}
