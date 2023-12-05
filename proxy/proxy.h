#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.h"

#define PORT 80
#define BUFFER_SIZE 1024
char* get_url(char* buffer);
char* get_refer_url(char* buffer);
void handle_client_request(int client_socket, Cache* cache);
int socket_init();
void set_params(struct sockaddr_in* server_addr);
void binding_and_listening(int server_socket, struct sockaddr_in* server_addr);
