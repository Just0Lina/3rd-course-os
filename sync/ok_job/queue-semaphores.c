#include "queue-semaphores.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void *qmonitor(void *arg) {
  queue_t *q = (queue_t *)arg;

  printf("qmonitor\n");

  while (1) {
    queue_print_stats(q);
    sleep(1);
  }

  return NULL;
}

queue_t *queue_init(int max_count) {
  queue_t *q = (queue_t *)malloc(sizeof(queue_t));
  if (!q) {
    perror("Cannot allocate memory for a queue");
    abort();
  }

  q->first = NULL;
  q->last = NULL;
  q->max_count = max_count;
  q->count = 0;

  q->add_attempts = 0;
  q->get_attempts = 0;
  q->add_count = 0;
  q->get_count = 0;
  int err;

  if (sem_init(&q->semaphore_items, 0, 0) != 0) {
    perror("Failed to initialize semaphore init");
    free(q);
    abort();
  }
  if (sem_init(&q->semaphore_spaces, 0, max_count) != 0) {
    perror("Failed to initialize semaphore spaces");
    free(q);
    abort();
  }
  err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);

  return q;
}

void queue_destroy(queue_t *q) {
  if (q) {
    sem_destroy(&q->semaphore_items);
    sem_destroy(&q->semaphore_spaces);
    while (q->first) {
      qnode_t *temp = q->first;
      q->first = q->first->next;
      free(temp);
    }

    free(q);
  }
}

int queue_add(queue_t *q, int val) {
  q->add_attempts++;

  assert(q->count <= q->max_count);

  sem_wait(&q->semaphore_spaces);

  qnode_t *new = (qnode_t *)malloc(sizeof(qnode_t));
  if (!new) {
    perror("Cannot allocate memory for a new node");
    abort();
  }

  new->val = val;
  new->next = NULL;

  if (!q->first) {
    q->first = q->last = new;
  } else {
    q->last->next = new;
    q->last = q->last->next;
  }

  q->count++;
  q->add_count++;

  sem_post(&q->semaphore_items);

  return 1;
}

int queue_get(queue_t *q, int *val) {
  q->get_attempts++;

  assert(q->count >= 0);

  sem_wait(&q->semaphore_items);

  qnode_t *tmp = q->first;

  *val = tmp->val;
  q->first = q->first->next;

  free(tmp);
  q->count--;
  q->get_count++;

  sem_post(&q->semaphore_spaces);

  return 1;
}

void queue_print_stats(queue_t *q) {
  printf(
      "queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld "
      "%ld)\n",
      q->count, q->add_attempts, q->get_attempts,
      q->add_attempts - q->get_attempts, q->add_count, q->get_count,
      q->add_count - q->get_count);
}
