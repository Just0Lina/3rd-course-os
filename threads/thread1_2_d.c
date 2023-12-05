#include <pthread.h>
#include <stdio.h>

void *mythread(void *arg) {
  // pthread_detach(pthread_self());  // e

  pthread_t tid = pthread_self();
  printf("Thread %lu is running\n", tid);
  pthread_exit(NULL);
}

int main() {
  while (1) {
    pthread_t tid;
    if (pthread_create(&tid, NULL, mythread, NULL) != 0) {
      perror("pthread_create");
      // break;  // Прерываем цикл в случае ошибки
    }

    // // pthread_create(&tid, NULL, mythread, NULL);

    // pthread_attr_t attr;  // f
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    // if (pthread_create(&tid, &attr, mythread, NULL) != 0) {
    //   perror("pthread_create");
    //   break;  // Прерываем цикл в случае ошибки
    // }
    // }

    return 0;
  }