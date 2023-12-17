#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include "proxy.h"

int server_socket;
Cache *cache;
int server_is_on = 1;

void signal_handler(int signum) {
  if (signum == SIGINT) {
    printf(ANSI_COLOR_RED
           "Received SIGINT, closing socket...\n" ANSI_COLOR_RESET);
    close(server_socket);
    server_is_on = 0;

    sem_destroy(&thread_semaphore);
    destroy_cache(cache);
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
  cache = (Cache *)malloc(sizeof(Cache));
  initialize_cache(cache);

  while (server_is_on) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                           &client_addr_len);
    if (client_socket < 0) {
      perror("Error accepting connection");
      continue;
    }

    handle_client_request(client_socket, cache);
  }

  close(server_socket);
  destroy_cache(cache);
  sem_destroy(&thread_semaphore);
  return 0;
}
