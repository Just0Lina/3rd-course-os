#ifndef CACHE_H
#define CACHE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_request.h"

#define MAX_CACHE_SIZE 2
#define MAX_URL_LEN 256
#define CACHE_BUFFER_SIZE (1024 * 1024 * 500)  // 500 MB

typedef struct {
  char url[MAX_URL_LEN];
  MemStruct* data;
  int count;
} CacheItem;

extern pthread_mutex_t cache_mutex;

typedef struct {
  CacheItem cache[MAX_CACHE_SIZE];
  int count;
} Cache;

void initialize_cache(Cache* cache);

MemStruct* get_data_from_cache(Cache* cache, const char* url);

void add_to_cache(Cache* cache, char* url, MemStruct* data);
void destroy_cache(Cache* cache);
#endif /* CACHE_H */
