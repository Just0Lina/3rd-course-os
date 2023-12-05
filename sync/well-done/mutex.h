#include <limits.h>
#include <linux/futex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

typedef struct {
  uint32_t futex;
} Mutex;

void mutex_init(Mutex *mutex) { mutex->futex = 0; }

void mutex_lock(Mutex *mutex) {
  while (__sync_lock_test_and_set(&mutex->futex, 1) != 0) {
    syscall(SYS_futex, &mutex->futex, FUTEX_WAIT, 1, NULL, NULL, 0);
  }
}

void mutex_unlock(Mutex *mutex) {
  __sync_lock_release(&mutex->futex);
  syscall(SYS_futex, &mutex->futex, FUTEX_WAKE, 1, NULL, NULL, 0);
}