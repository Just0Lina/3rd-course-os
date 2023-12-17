#include "cache.h"
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_cache(Cache* cache) { cache->count = 0; }

MemStruct* get_data_from_cache(Cache* cache, const char* url) {
  for (int i = 0; i < cache->count; ++i) {
    if (strcmp(cache->cache[i].url, url) == 0) {
      cache->cache[i].count++;
      return cache->cache[i].data;
    }
  }
  return NULL;
}
void destroy_cache(Cache* cache) {
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

void add_to_cache(Cache* cache, char* url, MemStruct* data) {
  pthread_mutex_lock(&cache_mutex);

  for (int i = 0; i < cache->count; ++i) {
    if (strcmp(cache->cache[i].url, url) == 0) {
      printf(ANSI_COLOR_YELLOW
             "Not add to cache url (already in) : |%s| \n" ANSI_COLOR_RESET,
             url);
      free(url);
      free(data->memory);
      free(data);
      pthread_mutex_unlock(&cache_mutex);
      return;
    }
  }
  if (cache->count < MAX_CACHE_SIZE) {
    printf(ANSI_COLOR_YELLOW
           "Add to cache url (count < max) : |%s| \n" ANSI_COLOR_RESET,
           url);
    strcpy(cache->cache[cache->count].url, url);
    free(url);
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
    free(url);
    free(cache->cache[minInd].data);
    cache->cache[minInd].data = data;
    cache->cache[minInd].count = 1;
  }
  pthread_mutex_unlock(&cache_mutex);
}
