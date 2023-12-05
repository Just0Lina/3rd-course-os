#include "cache.h"

void initializeCache(Cache* cache) { cache->count = 0; }

MemStruct* getDataFromCache(Cache* cache, const char* url) {
  for (int i = 0; i < cache->count; ++i) {
    if (strcmp(cache->cache[i].url, url) == 0) {
      cache->cache[i].count++;
      return cache->cache[i].data;
    }
  }
  return NULL;
}

void addToCache(Cache* cache, const char* url, MemStruct* data) {
  // printf("%d count \n", cache->count);
  // printf(ANSI_COLOR_GREEN "Add to cache url : |%s| \n" ANSI_COLOR_RESET,
  // url);
  //   printf(" data %s \n", data);

  if (cache->count < MAX_CACHE_SIZE) {
    printf(ANSI_COLOR_YELLOW
           "Add to cache url (count < max) : |%s| \n" ANSI_COLOR_RESET,
           url);
    strcpy(cache->cache[cache->count].url, url);
    cache->cache[cache->count].data = data;
    cache->cache[cache->count].count = 1;
    cache->count++;
  } else {
    printf(ANSI_COLOR_YELLOW
           "Add to cache url (delete) : |%s| \n" ANSI_COLOR_RESET,
           url);

    int minInd = 0;
    int min = cache->cache[0].count;

    for (int i = 1; i < cache->count; ++i) {
      if (cache->cache[i].count < min) {
        min = cache->cache[i].count;
        minInd = i;
      }
    }
    strcpy(cache->cache[minInd].url, url);
    // free(cache->cache[minInd].data);
    cache->cache[minInd].data = data;
    cache->cache[minInd].count = 1;
  }
}
