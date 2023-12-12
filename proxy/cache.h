#ifndef CACHE_H
#define CACHE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_request.h"

#define MAX_CACHE_SIZE 1000
#define MAX_URL_LEN 256
#define CACHE_BUFFER_SIZE (1024 * 1024 * 64) * 4  // 64 * 4 MB

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

void initializeCache(Cache* cache);

MemStruct* getDataFromCache(Cache* cache, const char* url);

void addToCache(Cache* cache, const char* url, MemStruct* data);
void destroyCache(Cache* cache);
#endif /* CACHE_H */
