#include <unistd.h>
#include "error.h"
#include "buffer.h"
#include "strerr.h"
#include "socket.h"

char ip[4];
int s;
int i;

int main() {
  ip[0] =127; ip[1] =0; ip[2] =0; ip[3] =1;
  s =socket_tcp();
  if (s == -1) strerr_die1sys(111, "fatal: unable to create socket: ");
  if (socket_bind4(s, ip, 0) == -1)
    strerr_die1sys(111, "fatal: unable to bind socket: ");
  if (socket_connect4(s, ip, 12614) == -1)
    if ((errno != error_wouldblock) && (errno != error_inprogress))
      strerr_die1sys(111, "fatal: unable to connect socket: ");
  for (i =0; i < 2; ++i) if (socket_connected(s)) break; else sleep(1);
  if (i >= 2) strerr_die1sys(111, "fatal: timeout connecting socket: ");
  if (write(s, "bar\n", 4) != 4)
    strerr_die1sys(111, "fatal: unable to write to socket: ");
  close(s);
  return(0);
}
