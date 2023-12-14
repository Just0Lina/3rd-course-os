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

void* fetchAndCacheData(void* arg) {
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
    close(client_socket);
  }
  printf(ANSI_COLOR_GREEN
         "Create new connection with remote server\n" ANSI_COLOR_RESET);

  ssize_t bytes_sent = write(dest_socket, request, strlen(request));
  if (bytes_sent == -1) {
    printf(ANSI_COLOR_RED
           "Error while sending request to remote server\n" ANSI_COLOR_RESET);
    close(client_socket);
    close(dest_socket);
    sem_post(&thread_semaphore);

    return NULL;
  }

  printf(ANSI_COLOR_GREEN
         "Send request to remote server, len = %ld\n" ANSI_COLOR_RESET,
         bytes_sent);

  char* buffer = calloc(BUFFER_SIZE, sizeof(char));
  ssize_t bytes_read, all_bytes_read = 0;
  while ((bytes_read = read(dest_socket, buffer, BUFFER_SIZE)) > 0) {
    // printf(ANSI_COLOR_GREEN
    //        "\tRead response from remote server, len = %d\n" ANSI_COLOR_RESET,
    //        bytes_read);
    bytes_sent = write(client_socket, buffer, bytes_read);
    if (bytes_sent == -1) {
      printf(ANSI_COLOR_RED
             "Error while sending data to client\n" ANSI_COLOR_RESET);
      // send_header_with_data(client_socket, record);

      close(client_socket);
      close(dest_socket);
      sem_post(&thread_semaphore);

      return NULL;
    } else {
      add_response(record, buffer, all_bytes_read, bytes_read);

      if (strstr(buffer, "ERROR") != NULL ||
          strstr(buffer, "Not Found") != NULL) {
        printf(ANSI_COLOR_RED
               "Server returned error, not saving to cache\n" ANSI_COLOR_RESET);
        send_header_with_data(client_socket, record);
        free(buffer);
        close(client_socket);
        close(dest_socket);
        sem_post(&thread_semaphore);

        return NULL;
      }
      // } else {
      //   // No error message found, proceed with caching
      //   add_response(record, buffer, all_bytes_read, bytes_read);
      //   // ... (rest of the caching logic)
      // }
      // printf(ANSI_COLOR_GREEN
      //        "\tWrite response to client, len = %d\n" ANSI_COLOR_RESET,
      //        bytes_sent);
      // add_response(record, buffer, all_bytes_read, bytes_read);
      // printf(ANSI_COLOR_GREEN
      //        "\t\tCached part of response, len = %d\n" ANSI_COLOR_RESET,
      //        bytes_sent);
    }
    all_bytes_read += bytes_read;
  }
  add_size(record, all_bytes_read);

  addToCache(cache, get_refer_url(request), record);

  printf("Data added to cache with size %ld\n\n", record->size);
  sem_post(&thread_semaphore);

  printf(ANSI_COLOR_MAGENTA "Sending data to client...\n" ANSI_COLOR_RESET);

  send_header_with_data(client_socket, record);
  printf(ANSI_COLOR_MAGENTA "Data sent to client.\n" ANSI_COLOR_RESET);

  close(client_socket);
  close(dest_socket);
  free(buffer);
  return NULL;
}

void send_header_with_data(int client_socket, MemStruct* data2) {
  ssize_t send_bytes = write(client_socket, data2->memory, data2->size);

  // send(client_socket, data2->memory, data2->size, 0);
}

void handle_client_request(int client_socket, Cache* cache) {
  printf(ANSI_COLOR_YELLOW "Handling client request...\n" ANSI_COLOR_RESET);
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
  if (bytes_read < 0) {
    perror("Error reading from socket");
    close(client_socket);
    return;
  }
  printf(buffer);

  char* url = get_refer_url(buffer);

  MemStruct* chunk = malloc(sizeof(MemStruct*));
  chunk->size = 30;
  chunk->memory = " ";

  if (url != NULL) {
    printf(ANSI_COLOR_CYAN "URL found: |%s|\n" ANSI_COLOR_RESET, url);

    MemStruct* data = getDataFromCache(cache, url);

    if (data != NULL) {
      printf(ANSI_COLOR_GREEN "Data found in cache!\n" ANSI_COLOR_RESET);
      send_header_with_data(client_socket, data);
    } else {
      printf(ANSI_COLOR_BLUE
             "Data not found in cache. Fetching from remote "
             "server...\n" ANSI_COLOR_RESET);

      sem_wait(&thread_semaphore);
      pthread_t tid;
      ThreadArgs args;
      args.cache = cache;
      args.request = buffer;
      args.client_socket = client_socket;
      printf(ANSI_COLOR_BLUE "Initing new thread\n" ANSI_COLOR_RESET);
      int err = pthread_create(&tid, NULL, &fetchAndCacheData, &args);
      if (err != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(err));
        return;
      }
      // void* result;
      // MemStruct* data2;
      // pthread_join(tid, &result);
      // if (result == NULL) {
      //   data2 = getDataFromCache(cache, url);

      //   // Thread function encountered an error
      //   // Handle the error condition
      // } else {
      //   // Thread function completed successfully, result points to the
      //   returned
      //   // data
      //   data2 = (MemStruct*)result;
      //   // Process the returned record
      // }
    }

  } else {
    printf(ANSI_COLOR_RED
           "URL is NULL. Sending default response...\n" ANSI_COLOR_RESET);
    // send_header_with_data(client_socket, chunk);
  }
  // printf(ANSI_COLOR_YELLOW "Closing client socket...\n" ANSI_COLOR_RESET);

  // close(client_socket);
}

char* extractReference(char* buffer, char* reference, char endChar) {
  const char* referer = strstr(buffer, reference);
  if (referer != NULL) {
    referer += strlen(reference);
    const char* endOfReferer = strchr(referer, endChar);
    if (endOfReferer != NULL) {
      int refererLength = endOfReferer - referer - 1;
      if (refererLength == 0) return NULL;
      char* extractedReferer = (char*)malloc(refererLength);
      strncpy(extractedReferer, referer, refererLength);
      return extractedReferer;
    }
  }
  return NULL;
}

char* get_refer_url(char* buffer) {
  char* url = extractReference(buffer, "GET ", 'H');
  // char* reference = "localhost";

  // if (strstr(url, reference) != NULL) {
  //   char* url2 = extractReference(buffer, "Sec-Fetch-Dest: ", '\n');
  //   char* doc = "document";
  //   if (strstr(url2, doc) == NULL) {
  //     url = extractReference(buffer, "localhost/", '\n');
  //   } else {
  //     url = extractReference(buffer, "GET /", 'H');
  //   }
  // }
  return url;
}