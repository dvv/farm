#include "zhelpers.h"

int main (int argc, char *argv [])
{
  void *context = zmq_init(1);

  // listening to messages
  fprintf(stderr, "Listener started at *:5554\n");
  void *sub = zmq_socket(context, ZMQ_SUB);
  zmq_bind(sub, "tcp://*:5554");
  zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);

  // publishing to subscribers
  fprintf(stderr, "Publisher started at *:5555\n");
  void *pub = zmq_socket(context, ZMQ_PUB);
  zmq_bind(pub, "tcp://*:5555");

  // loop
  while (1) {
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    // break if interrupted
    if (zmq_recv(sub, &msg, 0) != 0) break;
    /***
    // check if multipart
    int64_t more;
    size_t more_size = sizeof (more);
    zmq_getsockopt(sub, ZMQ_RCVMORE, &more, &more_size);
    if (!more) break; // last message part
    ***/
    zmq_send(pub, &msg, 0);
    zmq_msg_close(&msg);
  }

  // cleanup
  zmq_close(pub);
  zmq_close(sub);
  zmq_term(context);
  return 0;
}
