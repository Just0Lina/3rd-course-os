#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include "proxy.h"

int server_socket;

void signal_handler(int signum) {
  if (signum == SIGINT) {
    printf(ANSI_COLOR_RED
           "Received SIGINT, closing socket...\n" ANSI_COLOR_RESET);
    close(server_socket);
    sem_destroy(&thread_semaphore);
    exit(signum);
  }
}

int main() {
  int client_socket;
  signal(SIGINT, signal_handler);
  sem_init(&thread_semaphore, 0, MAX_USERS_COUNT);

  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  server_socket = socket_init();
  set_params(&server_addr);
  binding_and_listening(server_socket, &server_addr);
  Cache *cache = (Cache *)malloc(sizeof(Cache));
  initializeCache(cache);

  while (1) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                           &client_addr_len);
    if (client_socket < 0) {
      perror("Error accepting connection");
      continue;
    }

    handle_client_request(client_socket, cache);
  }

  close(server_socket);
  destroyCache(cache);
  sem_destroy(&thread_semaphore);
  return 0;
}
