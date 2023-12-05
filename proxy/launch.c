#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include "proxy.h"
// Cache cache;

int server_socket;

void signal_handler(int signum) {
  if (signum == SIGINT) {
    printf(ANSI_COLOR_RED
           "Received SIGINT, closing socket...\n" ANSI_COLOR_RESET);
    close(server_socket);
    exit(signum);
  }
}

int main() {
  int client_socket;
  signal(SIGINT, signal_handler);

  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  server_socket = socket_init();
  set_params(&server_addr);
  binding_and_listening(server_socket, &server_addr);
  Cache cache;
  initializeCache(&cache);

  // Основной цикл приема запросов клиентов
  while (1) {
    client_socket =
        accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket < 0) {
      perror("Error accepting connection");
      continue;
    }

    handle_client_request(client_socket, &cache);
  }

  close(server_socket);

  return 0;
}
