#include "http_request.h"

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb,
                                  void* userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct* mem = (struct MemoryStruct*)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    printf("Not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

MemStruct* sendHTTPRequest(const char* url) {
  CURL* curl;
  CURLcode res;
  char* httpURL = prependHTTPS(url);
  printf(ANSI_COLOR_BLUE "Https url: |%s|\n" ANSI_COLOR_RESET, httpURL);
  MemStruct* chunk = malloc(sizeof(MemStruct*));

  chunk->memory = malloc(1);
  chunk->size = 0;
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, httpURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)chunk);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  }
  free(httpURL);
  return chunk;
}

char* prependHTTPS(const char* url) {
  char* newURL = malloc(strlen(url) + strlen("https://"));
  if (newURL != NULL) {
    strcpy(newURL, "https://");
    strcat(newURL, url);
  }
  return newURL;
}
