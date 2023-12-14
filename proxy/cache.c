#include "cache.h"
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_cache(Cache* cache) { cache->count = 0; }

MemStruct* getDataFromCache(Cache* cache, const char* url) {
  // pthread_mutex_lock(&cache_mutex);
  for (int i = 0; i < cache->count; ++i) {
    if (strcmp(cache->cache[i].url, url) == 0) {
      cache->cache[i].count++;
      // pthread_mutex_unlock(&cache_mutex);
      return cache->cache[i].data;
    }
  }
  // pthread_mutex_unlock(&cache_mutex);
  return NULL;
}
void destroyCache(Cache* cache) {
  pthread_mutex_lock(&cache_mutex);

  if (cache == NULL) {
    return;
  }

  for (int i = 0; i < cache->count; ++i) {
    free(cache->cache[i].data->memory);
    free(cache->cache[i].data);
  }

  free(cache);
  pthread_mutex_unlock(&cache_mutex);
}

void addToCache(Cache* cache, const char* url, MemStruct* data) {
  // pthread_mutex_lock(&cache_mutex);
  for (int i = 0; i < cache->count; ++i) {
    if (strcmp(cache->cache[i].url, url) == 0) {
      printf(ANSI_COLOR_YELLOW
             "Not add to cache url (already in) : |%s| \n" ANSI_COLOR_RESET,
             url);
      return;
    }
  }
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
    cache->cache[minInd].data = data;
    cache->cache[minInd].count = 1;
  }
  // pthread_mutex_unlock(&cache_mutex);
}
