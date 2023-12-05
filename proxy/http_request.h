#define _GNU_SOURCE

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
typedef struct MemoryStruct {
  char* memory;
  size_t size;
} MemStruct;

MemStruct* sendHTTPRequest(const char* url);
char* prependHTTPS(const char* url);
