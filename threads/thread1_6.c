#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (4 * 1024 * 1024)

// Тип для идентификатора потока
typedef pid_t mythread_t;

// Тип для функции потока
typedef int (*mythread_start_routine)(void *);

void *mythread(void *arg) {
  printf("\n--------------------------\n");

  printf("mythread [%d %d %d]\n", getpid(), getppid(), gettid());
  return NULL;
}

int mythread_create(mythread_t *thread, mythread_start_routine function,
                    void *arg) {
  void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  *thread = clone(
      function, (char *)stack + STACK_SIZE,
      CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD, arg);

  if (*thread == -1) {
    perror("clone");
    munmap(stack, STACK_SIZE);
    return -1;
  }

  return 0;
}

void aLetter() {
  pthread_t tid;
  int err;
  err = pthread_create(&tid, NULL, mythread, NULL);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return;
  }
  pthread_join(tid, NULL);  // a
}

int my_thread_function(void *arg) {
  printf("Thread message = %s\n", (char *)arg);
  printf("mythread [%d %d %d]\n", getpid(), getppid(), gettid());
  //   sleep(2);
  return 0;
}

int main() {
  mythread_t thread;
  char *message = "I'm here";
  printf("Main [%d %d %d]!\n", getpid(), getppid(), gettid());
  if (mythread_create(&thread, my_thread_function, message) != 0) {
    fprintf(stderr, "Failed to create thread\n");
    return 1;
  }
  int status;
  waitpid(thread, &status, 0);
  sleep(2);
  aLetter();
  return 0;
}
