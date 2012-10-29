#include <stdint.h>
#include <stdio.h>
#include <zmq.h>

//
// Thanks to Neopallium at githib.com
// https://gist.github.com/2019204
//

int main(int argc, char *argv[])
{
  void *context = zmq_init(1);
  zmq_msg_t msg;
  int rc = 0;
  int64_t more;
  size_t more_size = sizeof more;

  // make sure msg is initialized
  zmq_msg_init(&msg);

  // listen to messages
  fprintf(stderr, "Push to *:65454\n");
  void *sub = zmq_socket(context, ZMQ_SUB);
  zmq_bind(sub, "tcp://*:65454");
  zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);

  // publish to subscribers
  fprintf(stderr, "Subscribe to *:65455\n");
  void *pub = zmq_socket(context, ZMQ_PUB);
  zmq_bind(pub, "tcp://*:65455");

  // loop until error
  while (rc != -1) {
    // receive message part
    rc = zmq_recvmsg(sub, &msg, 0);
    if (rc == -1) break; // break if interrupted
    // more parts are to follow?
    rc = zmq_getsockopt(sub, ZMQ_RCVMORE, &more, &more_size);
    if (rc == -1) break; // break if interrupted
    // relay
    rc = zmq_sendmsg(pub, &msg, more ? ZMQ_SNDMORE : 0);
  }
  if (rc == -1) {
    fprintf(stderr, "ERR: %d\n", zmq_errno());
  }

  // cleanup
  zmq_msg_close(&msg);
  zmq_close(pub);
  zmq_close(sub);
  zmq_term(context);

  return 0;
}
