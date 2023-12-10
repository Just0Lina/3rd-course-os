#include "proxy.h"

#include <pthread.h>
#include <semaphore.h>

// pthread_mutex_t cacheMutex = PTHREAD_MUTEX_INITIALIZER;
sem_t thread_semaphore;  // Definition of thread_semaphore

typedef struct {
  char* url;
  Cache* cache;
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

void set_params(struct sockaddr_in* server_addr) {  // Настройка адреса сервера
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

void* fetchAndCacheData(void* arg) {
  ThreadArgs* args = (ThreadArgs*)arg;
  Cache* cache = args->cache;
  char* url = args->url;
  MemStruct* chunk = sendHTTPRequest(url);
  addToCache(cache, url, chunk);
  printf("Data added to cache + %d\n\n", cache->count);
  sem_post(&thread_semaphore);

  return NULL;
}

void send_header_with_data(int client_socket, MemStruct* data2) {
  // const char* http_response =
  //     "HTTP/1.1 200 OK\r\n"
  //     "Content-Type: text/html\r\n";

  // char contentLengthHeader[100];
  // sprintf(contentLengthHeader, "Content-Length: %ld\r\n", data2->size);
  // printf(ANSI_COLOR_GREEN "Length: %d!\n" ANSI_COLOR_RESET, data2->size);

  // size_t http_response_len = strlen(http_response);
  // send(client_socket, http_response, http_response_len, 0);
  // send(client_socket, contentLengthHeader, strlen(contentLengthHeader), 0);
  // send(client_socket, "\r\n", strlen("\r\n"), 0);
  // send(client_socket, data2->memory, data2->size, 0);
  // send(client_socket, "\r\n", strlen("\r\n"), 0);
  const char* http_response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n";

  char contentLengthHeader[100];
  sprintf(contentLengthHeader, "Content-Length: %ld\r\n", data2->size);

  const char* empty_line = "\r\n";

  // Calculate the total length of the complete response
  size_t total_length = strlen(http_response) + strlen(contentLengthHeader) +
                        strlen(empty_line) + data2->size + strlen(empty_line);

  // Allocate memory for the complete response
  char* complete_response =
      (char*)malloc(total_length + 1);  // +1 for null terminator
  if (complete_response == NULL) {
    // Handle allocation failure
    return;
  }
  snprintf(complete_response, total_length + 1, "%s%s%s%s%s",
           (char*)http_response, (char*)contentLengthHeader, (char*)empty_line,
           (char*)data2->memory, (char*)empty_line);

  // Construct the complete response
  // snprintf(complete_response, total_length + 1, "%s%s%s%s%s%s",
  // http_response,
  //          contentLengthHeader, empty_line, data2->memory, empty_line);

  // Send the complete response
  send(client_socket, complete_response, total_length, 0);

  // Free the allocated memory
  free(complete_response);
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
      args.url = url;
      int err = pthread_create(&tid, NULL, fetchAndCacheData, &args);
      if (err != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(err));
        return;
      }
      pthread_join(tid, NULL);

      MemStruct* data2 = getDataFromCache(cache, url);

      printf(ANSI_COLOR_MAGENTA "Sending data to client...\n" ANSI_COLOR_RESET);

      send_header_with_data(client_socket, data2);
      printf(ANSI_COLOR_MAGENTA "Data sent to client.\n" ANSI_COLOR_RESET);
    }

  } else {
    printf(ANSI_COLOR_RED
           "URL is NULL. Sending default response...\n" ANSI_COLOR_RESET);
    send_header_with_data(client_socket, chunk);
  }
  printf(ANSI_COLOR_YELLOW "Closing client socket...\n" ANSI_COLOR_RESET);

  close(client_socket);
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
  char* url = extractReference(buffer, "Host: ", '\n');
  char* reference = "localhost";

  if (strstr(url, reference) != NULL) {
    char* url2 = extractReference(buffer, "Sec-Fetch-Dest: ", '\n');
    char* doc = "document";
    if (strstr(url2, doc) == NULL) {
      url = extractReference(buffer, "localhost/", '\n');
    } else {
      url = extractReference(buffer, "GET /", 'H');
    }
  }

  return url;
}