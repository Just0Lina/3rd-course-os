#include "queue-mutex-signal.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  int err;
  condition_t *condition = (condition_t *)malloc(sizeof(condition_t));
  if (!condition) {
    printf("Cannot allocate memory for condition variable\n");
    abort();
  }

  if (pthread_mutex_init(&condition->mutex, NULL) != 0) {
    printf("Failed to initialize mutex\n");
    free(condition);
    abort();
  }
  if (pthread_cond_init(&condition->cond, NULL) != 0) {
    printf("Failed to initialize condition variable\n");
    pthread_mutex_destroy(&condition->mutex);
    free(condition);
    abort();
  }

  queue_t *q = malloc(sizeof(queue_t));
  if (!q) {
    printf("Cannot allocate memory for a queue\n");
    abort();
  }

  q->first = NULL;
  q->last = NULL;
  q->max_count = max_count;
  q->count = 0;

  q->add_attempts = q->get_attempts = 0;
  q->add_count = q->get_count = 0;
  q->condition = condition;

  err = pthread_mutex_init(&q->lock, NULL);
  if (err) {
    printf("queue_init: pthread_mutex_init() failed: %s\n", strerror(err));
    abort();
  }

  err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
  if (err) {
    printf("queue_init: pthread_create() failed: %s\n", strerror(err));
    abort();
  }

  return q;
}

void queue_destroy(queue_t *q) {
  if (q) {
    condition_t *condition = q->condition;
    pthread_mutex_destroy(&condition->mutex);
    pthread_cond_destroy(&condition->cond);
    free(condition);
    pthread_cancel(q->qmonitor_tid);

    pthread_join(q->qmonitor_tid, NULL);

    while (q->first) {
      qnode_t *temp = q->first;
      q->first = q->first->next;
      free(temp);
    }
    pthread_mutex_destroy(&q->lock);

    free(q);
  }
}

int queue_add(queue_t *q, int val) {
  condition_t *condition = q->condition;

  pthread_mutex_lock(&condition->mutex);
  q->add_attempts++;
  while (q->count == q->max_count) {
    pthread_cond_wait(&condition->cond, &condition->mutex);
  }

  assert(q->count <= q->max_count);

  if (q->count == q->max_count) {
    pthread_mutex_unlock(&q->lock);
    return 0;
  }

  qnode_t *new = malloc(sizeof(qnode_t));
  if (!new) {
    printf("Cannot allocate memory for a new node\n");
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

  pthread_cond_signal(&condition->cond);

  pthread_mutex_unlock(&condition->mutex);
  return 1;
}

int queue_get(queue_t *q, int *val) {
  condition_t *condition = q->condition;

  pthread_mutex_lock(&condition->mutex);
  q->get_attempts++;

  while (q->count == 0) {
    pthread_cond_wait(&condition->cond, &condition->mutex);
  }

  assert(q->count >= 0);

  if (q->count == 0) {
    pthread_mutex_unlock(&q->lock);
    return 0;
  }

  qnode_t *tmp = q->first;

  *val = tmp->val;
  q->first = q->first->next;

  free(tmp);
  q->count--;
  q->get_count++;

  pthread_cond_signal(&condition->cond);

  pthread_mutex_unlock(&condition->mutex);
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
