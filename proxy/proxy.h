#ifndef PROXY_H
#define PROXY_H
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.h"
#define MAX_USERS_COUNT 10
extern sem_t thread_semaphore;

#define PORT 80
#define BUFFER_SIZE 4096
char* get_refer_url(char* buffer);
void handle_client_request(int client_socket, Cache* cache);
int socket_init();
void set_params(struct sockaddr_in* server_addr);
void binding_and_listening(int server_socket, struct sockaddr_in* server_addr);
void send_header_with_data(int client_socket, MemStruct* data2);
#endif /* PROXY_H */
