#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int var = 0;
void *mythread(void *arg) {
  while (1) {
    printf("Thread is running\n");
    sleep(1);
  }
  return NULL;
}

void *mythreadExit(void *arg) {
  while (1) {
    pthread_testcancel();
    var++;
    sleep(1);
  }
  return NULL;
}

void cleanup_handler2(void *arg) {
  char *str = (char *)arg;
  free(str);
}
void cleanup_handler(void *arg) { printf("%s!", "WWWWWWWWWWWWW"); }

void *mythreadHandler(void *arg) {
  char *str = "hello world";
  char *str_copy = (char *)malloc(strlen(str) + 1);
  if (str == NULL) {
    perror("malloc");
    pthread_exit(NULL);
  }
  strcpy(str_copy, str);
  pthread_cleanup_push(cleanup_handler, str_copy);
  pthread_cleanup_push(cleanup_handler2, str_copy);

  while (1) {
    printf("%s\n", str);
  }
  pthread_cleanup_pop(1);  // Выполнить cleanup_handler при отмене потока

  pthread_cleanup_pop(1);  // Выполнить cleanup_handler при отмене потока
  return NULL;
}

int main() {
  pthread_t tid;

  //   if (pthread_create(&tid, NULL, mythread, NULL) != 0) {
  //     perror("pthread_create");
  //     return 1;
  //   }

  //   if (pthread_create(&tid, NULL, mythreadExit, NULL) != 0) {
  //     perror("pthread_create");
  //     return 1;
  //   }
  //   sleep(4);
  //   printf("%d\n", var);

  if (pthread_create(&tid, NULL, mythreadHandler, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  sleep(3);

  if (pthread_cancel(tid) != 0) {
    perror("pthread_cancel");
    return 1;
  }

  if (pthread_join(tid, NULL) != 0) {
    perror("pthread_join");
    return 1;
  }

  return 0;
}
