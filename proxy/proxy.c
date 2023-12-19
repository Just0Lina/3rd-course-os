#include "proxy.h"

#include <pthread.h>
#include <semaphore.h>

sem_t thread_semaphore;

typedef struct {
  char* request;
  Cache* cache;
  int client_socket;
} ThreadArgs;

int socket_init() {
  int option = 1;
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  return server_socket;
}

void set_params(struct sockaddr_in* server_addr) {
  memset(server_addr, 0, sizeof(*server_addr));
  server_addr->sin_family = AF_INET;
  server_addr->sin_addr.s_addr = INADDR_ANY;
  server_addr->sin_port = htons(PORT);
}

void binding_and_listening(int server_socket, struct sockaddr_in* server_addr) {
  if (bind(server_socket, (struct sockaddr*)server_addr, sizeof(*server_addr)) <
      0) {
    perror("Error binding socket");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket, MAX_USERS_COUNT) < 0) {
    perror("Error listening on socket");
    exit(EXIT_FAILURE);
  }

  printf(ANSI_COLOR_GREEN
         "Proxy server started. Listening on port %d...\n" ANSI_COLOR_RESET,
         PORT);
}

void add_response(MemStruct* record, char* resp, unsigned long cur_position,
                  unsigned long resp_size) {
  memcpy(record->memory + cur_position, resp, resp_size);
}

void add_size(MemStruct* record, ssize_t size) { record->size = size; }

void* fetch_and_cache_data(void* arg) {
  ThreadArgs* args = (ThreadArgs*)arg;
  Cache* cache = args->cache;
  char* request = args->request;
  int client_socket = args->client_socket;
  MemStruct* record = malloc(sizeof(MemStruct));
  record->memory = (char*)calloc(CACHE_BUFFER_SIZE, sizeof(char));
  record->size = 0;
  unsigned char host[50];
  const unsigned char* host_result =
      memccpy(host, strstr((char*)request, "Host:") + 6, '\r', sizeof(host));
  host[host_result - host - 1] = '\0';
  int dest_socket = connect_to_remote((char*)host);
  if (dest_socket == -1) {
    printf(ANSI_COLOR_RED "Dest socket error\n" ANSI_COLOR_RESET);
    close(client_socket);
  }
  printf(ANSI_COLOR_GREEN
         "Create new connection with remote server\n" ANSI_COLOR_RESET);

  ssize_t bytes_sent = write(dest_socket, request, strlen(request));
  if (bytes_sent == -1) {
    printf(ANSI_COLOR_RED
           "Error while sending request to remote server\n" ANSI_COLOR_RESET);

    free(record->memory);
    free(record);
    close(client_socket);
    close(dest_socket);
    sem_post(&thread_semaphore);
    return NULL;
  }

  printf(ANSI_COLOR_GREEN
         "Send request to remote server, len = %ld\n" ANSI_COLOR_RESET,
         bytes_sent);

  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  ssize_t bytes_read, all_bytes_read = 0;
  while ((bytes_read = read(dest_socket, buffer, BUFFER_SIZE)) > 0) {
    // printf(ANSI_COLOR_GREEN
    //        "\tRead response from remote server, len = %ld\n"
    //        ANSI_COLOR_RESET, bytes_read);
    // printf(ANSI_COLOR_GREEN "\tBuffer: %s\n" ANSI_COLOR_RESET, buffer);
    bytes_sent = write(client_socket, buffer, bytes_read);
    if (bytes_sent == -1) {
      if (errno == EPIPE) {
        printf(ANSI_COLOR_RED
               "Error while sending data to client\n" ANSI_COLOR_RESET);
        free(record->memory);
        free(record);
        close(client_socket);
        close(dest_socket);
        sem_post(&thread_semaphore);
        return NULL;
      } else {
        add_response(record, buffer, all_bytes_read, bytes_read);

        if (strstr(buffer, "ERROR") != NULL ||
            strstr(buffer, "Not Found") != NULL) {
          printf(
              ANSI_COLOR_RED
              "Server returned error, not saving to cache\n" ANSI_COLOR_RESET);
          send_header_with_data(client_socket, record);
          // free(buffer);
          free(record->memory);
          free(record);
          close(client_socket);
          close(dest_socket);
          sem_post(&thread_semaphore);
          return NULL;
        }
      }
      all_bytes_read += bytes_read;
    }
    add_size(record, all_bytes_read);
    char* ref = get_refer_url(request);
    add_to_cache(cache, ref, record);

    printf(ANSI_COLOR_MAGENTA "Sending data to client...\n" ANSI_COLOR_RESET);
    printf("%s\n", ref);
    MemStruct* mem = get_data_from_cache(cache, ref);
    send_header_with_data(client_socket, mem);
    printf(ANSI_COLOR_MAGENTA "Data sent to client.\n" ANSI_COLOR_RESET);
    close(client_socket);
    close(dest_socket);
    sem_post(&thread_semaphore);
    free(ref);
    return NULL;
  }

  void send_header_with_data(int client_socket, MemStruct* data2) {
    write(client_socket, data2->memory, data2->size);
  }

  void handle_client_request(int client_socket, Cache* cache) {
    printf(ANSI_COLOR_YELLOW "Handling client request...\n" ANSI_COLOR_RESET);

    char* buffer = calloc(BUFFER_SIZE, sizeof(char));

    int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_read < 0) {
      perror("Error reading from socket");
      close(client_socket);
      free(buffer);
      return;
    }
    // printf("%s\n", buffer);

    char* url =
        extractReference(buffer, "GET ", 'H');  // get_refer_url(buffer);

    if (url != NULL) {
      // printf(ANSI_COLOR_CYAN "URL found: |%s|\n" ANSI_COLOR_RESET, url);
      MemStruct* data = get_data_from_cache(cache, url);

      if (data != NULL) {
        printf(ANSI_COLOR_GREEN "Data found in cache!\n" ANSI_COLOR_RESET);
        send_header_with_data(client_socket, data);
        close(client_socket);
      } else {
        printf(ANSI_COLOR_BLUE
               "Data not found in cache. Fetching from remote "
               "server...\n" ANSI_COLOR_RESET);

        sem_wait(&thread_semaphore);
        pthread_t tid;
        ThreadArgs* args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
        args->cache = cache;
        args->request = strdup(buffer);
        args->client_socket = client_socket;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        printf(ANSI_COLOR_BLUE "Initing new thread\n" ANSI_COLOR_RESET);
        int err = pthread_create(&tid, NULL, &fetch_and_cache_data, args);
        if (err != 0) {
          fprintf(stderr, "Error creating thread: %s\n", strerror(err));
          free(args->request);
          free(args);
          free(buffer);
          free(url);
          close(client_socket);
          return;
        }
      }

    } else {
      printf(ANSI_COLOR_RED
             "URL is NULL. Sending default response...\n" ANSI_COLOR_RESET);
      close(client_socket);
    }
    printf(ANSI_COLOR_YELLOW "Closing client socket!...\n" ANSI_COLOR_RESET);
    free(url);
    free(buffer);
  }

  char* extractReference(char* buffer, char* reference, char endChar) {
    const char* referer = strstr(buffer, reference);
    if (referer != NULL) {
      referer += strlen(reference);
      const char* endOfReferer = strchr(referer, endChar);
      if (endOfReferer != NULL) {
        int refererLength = endOfReferer - referer - 1;
        if (refererLength == 0) {
          free(buffer);
          return NULL;
        }
        char* extractedReferer = (char*)malloc(refererLength + 1);
        strncpy(extractedReferer, referer, refererLength);
        extractedReferer[refererLength] = '\0';
        return extractedReferer;
      }
    }
    return NULL;
  }

  char* get_refer_url(char* buffer) {
    return extractReference(buffer, "GET ", 'H');
  }