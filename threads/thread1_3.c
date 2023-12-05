#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // Для функции sleep

struct ThreadParams {
  int num;
  const char *message;
};

void *mythread(void *arg) {
  struct ThreadParams *params = (struct ThreadParams *)arg;
  printf("Thread %d: %s\n", params->num, params->message);
  pthread_exit(NULL);
}

int main() {
  // struct ThreadParams params = {1, "Hello from thread"};

  // pthread_t tid;

  // if (pthread_create(&tid, NULL, mythread, &params) != 0) {
  //   perror("pthread_create");
  //   return 1;
  // }

  // if (pthread_join(tid, NULL) != 0) {
  //   perror("pthread_join");
  //   return 1;
  // }

  struct ThreadParams *params =
      (struct ThreadParams *)malloc(sizeof(struct ThreadParams));
  params->num = 1;
  params->message = "Hello from thread";
  pthread_attr_t attr;
  pthread_t tid;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&tid, &attr, mythread, params) != 0) {
    perror("pthread_create");
    return 1;
  }
  sleep(2);
  return 0;
}
