#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int globalVar = 100;

void *mythread(void *arg) {
  printf("\n--------------------------\n");

  printf("mythread [%d %d %d]\n", getpid(), getppid(), gettid());
  pid_t pid = getpid();
  pid_t ppid = getppid();
  pthread_t ptid = pthread_self();
  pid_t tid = gettid();

  printf("mythread [%d %d %ld %d]\n", pid, ppid, ptid, tid);

  if (pthread_equal(ptid, *(pthread_t *)arg)) {
    printf("mythread: %ld  matches pthread_create() result.\n", pthread_self());
  } else {
    printf("mythread: %ld does not match pthread_create() result.\n",
           pthread_self());
  }
  int localVar = 42;
  static int staticLocalVar = 100;
  const int constLocalVar = 200;
  printf("bLetter: Local Variable Address: %p\n", (void *)&localVar);
  printf("bLetter: Static Local Variable Address: %p\n",
         (void *)&staticLocalVar);
  printf("bLetter: Constant Local Variable Address: %p\n",
         (void *)&constLocalVar);
  printf("bLetter: Global Variable Address: %d %p\n", globalVar,
         (void *)&globalVar);

  localVar = 234;
  globalVar = 3334;
  printf("mythread: Local Variable (Modified): %d\n", localVar);
  printf("mythread: Global Variable (Modified): %d\n", globalVar);
  return NULL;
}

void aLetter() {
  pthread_t tid;
  int err;

  printf("main [%d %d %d]\n", getpid(), getppid(), gettid());

  err = pthread_create(&tid, NULL, mythread, NULL);

  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return;
  }
  // pthread_join(tid, NULL);  // a
}

void bLetter() {
  pthread_t tid[5];
  int err;

  printf("Main [%d %d %d]\n", getpid(), getppid(), gettid());
  for (int i = 0; i < 5; ++i) {
    err = pthread_create(&tid[i], NULL, mythread, &tid[i]);
    if (err) {
      printf("main: pthread_create() failed: %s\n", strerror(err));
      return;
    }
    // pthread_join(tid[i], NULL);  // a
  }
}

int main() {
  //   aLetter();
  bLetter();
  sleep(10000);
  return 0;
}
