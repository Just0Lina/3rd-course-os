#include "proxy.h"

#include <pthread.h>

pthread_mutex_t cacheMutex = PTHREAD_MUTEX_INITIALIZER;

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

  if (listen(server_socket, 5) < 0) {
    perror("Error listening on socket");
    exit(EXIT_FAILURE);
  }
  printf(ANSI_COLOR_GREEN
         "Proxy server started. Listening on port %d...\n" ANSI_COLOR_RESET,
         PORT);
}

void sendHeader(int client_socket) {
  const char* http_response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "\r\n";

  size_t http_response_len = strlen(http_response);
  send(client_socket, http_response, http_response_len, 0);
}
// void* fetchAndCacheData(void* arg) {
//   ThreadArgs* args = (ThreadArgs*)arg;
//   Cache* cache = args->cache;
//   char* url = args->url;
//   // Здесь выполняется загрузка данных с удаленного сервера

//   // Предположим, данные были загружены в переменную "data"

//   // printf("Data not found in cache + %d\n", cache->count);
//   MemStruct* chunk = sendHTTPRequest(url);
//   // send(client_socket, chunk->memory, chunk->size, 0);
//   // pthread_mutex_lock(&cacheMutex);

//   addToCache(cache, url, chunk);  // to do add result data
//   printf("Data added to cache + %d\n\n", cache->count);
//   // Добавим данные в кэш
//   // pthread_mutex_unlock(&cacheMutex);

//   return NULL;
// }

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
  // printf("%s\n", buffer);
  char* url = get_refer_url(buffer);

  // printf("%s\n", url);
  MemStruct* chunk = malloc(sizeof(MemStruct*));
  chunk->size = 30;
  chunk->memory = "HELLO WOrLD";

  if (url != NULL) {
    printf(ANSI_COLOR_CYAN "URL found: %s\n" ANSI_COLOR_RESET, url);

    pthread_mutex_lock(&cacheMutex);
    MemStruct* data = getDataFromCache(cache, url);
    pthread_mutex_unlock(&cacheMutex);

    if (data != NULL) {
      printf(ANSI_COLOR_GREEN
             "Data found in cache!\n" ANSI_COLOR_RESET);  //,data->memory);
      sendHeader(client_socket);
      send(client_socket, data->memory, data->size, 0);
    } else {
      printf(ANSI_COLOR_BLUE
             "Data not found in cache. Fetching from remote "
             "server...\n" ANSI_COLOR_RESET);

      // pthread_t tid;
      // ThreadArgs args;
      // args.cache = cache;
      // args.url = url;

      chunk = sendHTTPRequest(url);

      printf(ANSI_COLOR_MAGENTA "Sending data to client...\n" ANSI_COLOR_RESET);
      sendHeader(client_socket);
      send(client_socket, chunk->memory, chunk->size, 0);
      // pthread_mutex_lock(&cacheMutex);
      printf(ANSI_COLOR_MAGENTA "Data sent to client.\n" ANSI_COLOR_RESET);

      addToCache(cache, url, chunk);  // to do add result data
      // fetchAndCacheData(&args);
      // int err = pthread_create(&tid, NULL, fetchAndCacheData, &args);
      // if (err != 0) {
      //   fprintf(stderr, "Error creating thread: %s\n", strerror(err));
      //   return;
      // }
      // pthread_join(tid, NULL);
      // pthread_mutex_lock(&cacheMutex);

      // MemStruct* data2 = getDataFromCache(cache, url);
      // if (data2 == NULL)
      //   printf("Piece of shit");
      // else
      //   printf("Try to send\n");
      // printf(data2->memory);
      // send(client_socket, data2->memory, data2->size, 0);
      // pthread_mutex_unlock(&cacheMutex);
    }
  } else {
    printf(ANSI_COLOR_RED
           "URL is NULL. Sending default response...\n" ANSI_COLOR_RESET);
    // send(client_socket, chunk->memory, chunk->size, 0);
  }
  printf(ANSI_COLOR_YELLOW "Closing client socket...\n" ANSI_COLOR_RESET);

  close(client_socket);
}

char* get_url(char* httpRequest) {
  const char* space = strchr(httpRequest, ' ');
  if (space != NULL) {
    const char* nextSpace = strchr(space + 1, ' ');
    if (nextSpace != NULL) {
      size_t urlLength = nextSpace - (space + 2);

      char* url = (char*)malloc(urlLength);
      strncpy(url, space + 1, urlLength);
      return url;
    }
  }

  return NULL;
}

char* get_refer_url(char* buffer) {
  const char* referer = strstr(buffer, "Host: ");
  if (referer != NULL) {
    referer += strlen("Host: ");
    const char* endOfReferer = strchr(referer, '\n');
    if (endOfReferer != NULL) {
      int refererLength = endOfReferer - referer - 1;
      if (refererLength == 0) return NULL;
      char* extractedReferer = (char*)malloc(refererLength);
      strncpy(extractedReferer, referer, refererLength);
      // printf("Extracted Referer: |%s|\n", extractedReferer);
      return extractedReferer;
      // }
    }
  }
  return NULL;
}