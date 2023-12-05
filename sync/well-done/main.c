#include <linux/futex.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>

#include "mutex.h"
#include "spin.h"

const int N = 5;
const int ITERATIONS = 100000;
int sharedCounter = 0;

void* incrementThreadSpin(void* arg) {
  Spinlock* lock = (Spinlock*)arg;
  for (int i = 0; i < ITERATIONS; ++i) {
    spinlock_lock(lock);
    sharedCounter++;
    spinlock_unlock(lock);
  }

  return NULL;
}

void* incrementThreadMutex(void* arg) {
  Mutex* mutex = (Mutex*)arg;

  for (int i = 0; i < ITERATIONS; ++i) {
    mutex_lock(mutex);
    sharedCounter++;
    mutex_unlock(mutex);
  }

  return NULL;
}

int main() {
  Spinlock myLock;
  spinlock_init(&myLock);

  pthread_t threads_spin[N];
  for (int i = 0; i < N; ++i) {
    pthread_create(&threads_spin[i], NULL, incrementThreadSpin, (void*)&myLock);
  }

  for (int i = 0; i < N; ++i) {
    pthread_join(threads_spin[i], NULL);
  }
  printf("The size of sharedCounter: %d == %d N * ITERATIONS\n", sharedCounter,
         N * ITERATIONS);
  sharedCounter = 0;

  Mutex myMutex;
  mutex_init(&myMutex);

  pthread_t threads_mutex[N];
  for (int i = 0; i < N; ++i) {
    pthread_create(&threads_mutex[i], NULL, incrementThreadMutex,
                   (void*)&myMutex);
  }

  for (int i = 0; i < N; ++i) {
    pthread_join(threads_mutex[i], NULL);
  }
  printf("The size of sharedCounter: %d == %d N * ITERATIONS\n", sharedCounter,
         N * ITERATIONS);
  sharedCounter = 0;

  return 0;
}
