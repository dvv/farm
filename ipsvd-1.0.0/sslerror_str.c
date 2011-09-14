#include "matrixSsl.h"

#define X(e,s) if (i == e) return s;

const char *sslerror_str(int i) {
  X(SSL_ALERT_CLOSE_NOTIFY, "close notify");
  X(SSL_ALERT_UNEXPECTED_MESSAGE, "unexpected message");
  X(SSL_ALERT_BAD_RECORD_MAC, "bad record mac");
  X(SSL_ALERT_DECOMPRESSION_FAILURE, "decompression failure");
  X(SSL_ALERT_HANDSHAKE_FAILURE, "handshake failure");
  X(SSL_ALERT_NO_CERTIFICATE, "no certificate");
  X(SSL_ALERT_BAD_CERTIFICATE, "bad certificate");
  X(SSL_ALERT_UNSUPPORTED_CERTIFICATE, "unsupported certificate");
  X(SSL_ALERT_CERTIFICATE_REVOKED, "certificate revoked");
  X(SSL_ALERT_CERTIFICATE_EXPIRED, "certificate expired");
  X(SSL_ALERT_CERTIFICATE_UNKNOWN, "certificate unknown");
  X(SSL_ALERT_ILLEGAL_PARAMETER, "illegal parameter");
  return "unknown alert";
}
