#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  volatile uint32_t lock;
} Spinlock;

void spinlock_init(Spinlock *lock) { lock->lock = 0; }

void spinlock_lock(Spinlock *lock) {
  while (__sync_lock_test_and_set(&lock->lock, 1)) {
  }
}

void spinlock_unlock(Spinlock *lock) { __sync_lock_release(&lock->lock); }