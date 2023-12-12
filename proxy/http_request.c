#include "http_request.h"

int connect_to_remote(char* host) {
  struct addrinfo hints, *res0;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo((char*)host, "http", &hints, &res0);
  if (status != 0) {
    freeaddrinfo(res0);
    return -1;
  }
  int dest_socket =
      socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);
  if (dest_socket == -1) {
    printf(ANSI_COLOR_RED
           "Error while creating remote server socket\n" ANSI_COLOR_RESET);
    return -1;
  }

  int err = connect(dest_socket, res0->ai_addr, res0->ai_addrlen);
  if (err == -1) {
    printf(ANSI_COLOR_RED
           "Error while connecting to remote server socket\n" ANSI_COLOR_RESET);
    close(dest_socket);
    freeaddrinfo(res0);
    return -1;
  }
  return dest_socket;
}
