#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *mythread(void *arg) {
  printf("Child thread is running\n");
  pthread_exit(NULL);
}

void *mythread42(void *arg) {
  int *result = (int *)malloc(sizeof(int));
  if (result == NULL) {
    perror("malloc");
    pthread_exit(NULL);
  }

  *result = 42;
  pthread_exit(result);
}

void *mythreadString(void *arg) {
  char *message = "hello world";
  pthread_exit((void *)message);
}

int main() {
  pthread_t tid;
  pthread_create(&tid, NULL, mythread, NULL);
  pthread_join(tid, NULL);
  printf("Main thread: Child thread has finished\n");

  void *thread_result;
  pthread_create(&tid, NULL, mythread42, NULL);
  pthread_join(tid, &thread_result);
  int *result = (int *)thread_result;
  printf("Main thread received result: %d\n", *result);

  void *thread_result_string;

  pthread_create(&tid, NULL, mythreadString, NULL);
  pthread_join(tid, &thread_result_string);

  char *message = (char *)thread_result_string;
  printf("Main thread received message: %s\n", message);

  return 0;
}
