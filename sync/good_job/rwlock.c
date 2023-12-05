#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_STRING_LENGTH 100
#define VALUES 10000

int iterations_increase = 0;
int iterations_decrease = 0;
int iterations_equal = 0;
int swap_count = 0;

typedef struct _Node {
  char value[MAX_STRING_LENGTH];
  struct _Node *next;
  pthread_rwlock_t sync;
} Node;

typedef struct _Storage {
  Node *first;
  pthread_rwlock_t storage_lock;
} Storage;

int getRandomNumber(int min, int max) { return rand() % (max - min + 1) + min; }

char *generateRandomString() {
  int length = getRandomNumber(1, MAX_STRING_LENGTH);
  //   printf("%d ", length);
  char *str = (char *)malloc((length + 1) * sizeof(char));

  if (str != NULL) {
    for (int i = 0; i < length; ++i) {
      str[i] = 'a' + rand() % 26;
    }
    str[length] = '\0';
  }

  return str;
}

Node *createNode(const char *value) {
  Node *newNode = (Node *)malloc(sizeof(Node));
  if (newNode != NULL) {
    strncpy(newNode->value, value, MAX_STRING_LENGTH - 1);
    newNode->value[MAX_STRING_LENGTH - 1] = '\0';
    newNode->next = NULL;
    pthread_rwlock_init(&newNode->sync, NULL);
  }
  return newNode;
}

void addNode(Storage *storage, const char *value) {
  Node *newNode = createNode(value);

  if (newNode == NULL) {
    fprintf(stderr, "Failed to create a new node.\n");
    return;
  }

  pthread_rwlock_wrlock(&storage->storage_lock);
  pthread_rwlock_wrlock(&newNode->sync);
  if (storage->first == NULL) {
    storage->first = newNode;
  } else {
    Node *current = storage->first;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = newNode;
  }
  pthread_rwlock_unlock(&newNode->sync);
  pthread_rwlock_unlock(&storage->storage_lock);
}

void initStorage(Storage *storage, int n) {
  srand(time(NULL));

  storage->first = NULL;
  pthread_rwlock_init(&storage->storage_lock, NULL);
  for (int i = 0; i < n; ++i) {
    addNode(storage, generateRandomString());
  }
}

void freeStorage(Storage *storage) {
  Node *current = storage->first;
  while (current != NULL) {
    // printf("%s \n", current->value);
    Node *temp = current;
    current = current->next;
    pthread_rwlock_destroy(&temp->sync);
    free(temp);
  }
  pthread_rwlock_destroy(&storage->storage_lock);
}

void *findIncreasingLengthStrings(void *arg) {
  Storage *storage = (Storage *)arg;
  while (1) {
    usleep(500);

    Node *current = storage->first;
    int count1 = 0;
    pthread_rwlock_wrlock(&storage->storage_lock);
    do {
      pthread_rwlock_rdlock(&current->sync);
      pthread_rwlock_rdlock(&current->next->sync);
      if (strlen(current->value) < strlen(current->next->value)) {
        count1++;
      }
      pthread_rwlock_unlock(&current->sync);
      pthread_rwlock_unlock(&current->next->sync);
      current = current->next;
    } while (current != NULL && current->next != NULL);

    iterations_increase++;
    printf("Thread increase: Iteration: %d Got: %d\n", iterations_increase,
           count1);
    pthread_rwlock_unlock(&storage->storage_lock);
  }

  return NULL;
}

void *findDecreasingLengthStrings(void *arg) {
  Storage *storage = (Storage *)arg;
  while (1) {
    usleep(500);

    Node *current = storage->first;
    int count = 0;
    do {
      pthread_rwlock_rdlock(&current->sync);
      pthread_rwlock_rdlock(&current->next->sync);
      if (strlen(current->value) > strlen(current->next->value)) {
        count++;
      }
      pthread_rwlock_unlock(&current->sync);
      pthread_rwlock_unlock(&current->next->sync);
      current = current->next;
    } while (current != NULL && current->next != NULL);

    iterations_decrease++;
    printf("Thread decrease: Iteration: %d Got: %d\n", iterations_decrease,
           count);
  }

  return NULL;
}
void *findEqualLengthStrings(void *arg) {
  Storage *storage = (Storage *)arg;
  while (1) {
    usleep(500);

    Node *current = storage->first;
    int count = rand() % (VALUES - 4);

    do {
      pthread_rwlock_rdlock(&current->sync);
      pthread_rwlock_rdlock(&current->next->sync);

      if (strlen(current->value) == strlen(current->next->value)) {
        count++;
      }
      pthread_rwlock_unlock(&current->sync);
      pthread_rwlock_unlock(&current->next->sync);
      current = current->next;
    } while (current != NULL && current->next != NULL);

    iterations_equal++;
    printf("Thread equal: Iteration: %d Got: %d\n", iterations_equal, count);
  }

  return NULL;
}

void *swapNodesIncrease(void *arg) {
  Storage *storage = (Storage *)arg;

  while (1) {
    usleep(500);
    Node *current = storage->first;
    int count = rand() % (VALUES - 4);
    int n = 0;
    do {
      if (count == n) {
        pthread_rwlock_wrlock(&current->sync);
        pthread_rwlock_wrlock(&current->next->sync);
        pthread_rwlock_wrlock(&current->next->next->sync);
        Node *temp1 = current->next;
        Node *temp2 = current->next->next;
        Node *temp0 = current;
        Node *temp3 = current->next->next->next;

        if (strlen(current->value) < strlen(current->next->value)) {
          temp0->next = temp2;
          temp2->next = temp1;
          temp1->next = temp3;
          swap_count++;
          printf("Changed increase %d\n", swap_count);
        }

        pthread_rwlock_unlock(&temp0->sync);
        pthread_rwlock_unlock(&temp1->sync);
        pthread_rwlock_unlock(&temp2->sync);
        break;
      }
      current = current->next;
      n++;
    } while (current != NULL && current->next != NULL &&
             current->next->next != NULL);
  }
}

void *swapNodesDecrease(void *arg) {
  Storage *storage = (Storage *)arg;

  while (1) {
    usleep(500);
    Node *current = storage->first;
    int count = rand() % (VALUES - 4);
    int n = 0;
    do {
      if (count == n) {
        pthread_rwlock_wrlock(&current->sync);
        pthread_rwlock_wrlock(&current->next->sync);
        pthread_rwlock_wrlock(&current->next->next->sync);
        Node *temp1 = current->next;
        Node *temp2 = current->next->next;
        Node *temp0 = current;
        Node *temp3 = current->next->next->next;

        if (strlen(current->value) > strlen(current->next->value)) {
          //   printf("JERER\n");

          temp0->next = temp2;
          temp2->next = temp1;
          temp1->next = temp3;
          swap_count++;
          printf("Changed decrease %d\n", swap_count);
        }

        pthread_rwlock_unlock(&temp0->sync);
        pthread_rwlock_unlock(&temp1->sync);
        pthread_rwlock_unlock(&temp2->sync);
        break;
      }
      current = current->next;
      n++;
    } while (current != NULL && current->next != NULL &&
             current->next->next != NULL);
  }
}

void *swapNodesEqual(void *arg) {
  Storage *storage = (Storage *)arg;

  while (1) {
    usleep(500);
    Node *current = storage->first;
    int count = rand() % (VALUES - 4);
    int n = 0;
    do {
      if (count == n) {
        pthread_rwlock_wrlock(&current->sync);
        pthread_rwlock_wrlock(&current->next->sync);
        pthread_rwlock_wrlock(&current->next->next->sync);
        Node *temp1 = current->next;
        Node *temp2 = current->next->next;
        Node *temp0 = current;
        Node *temp3 = current->next->next->next;

        if (strlen(current->value) == strlen(current->next->value)) {
          temp0->next = temp2;
          temp2->next = temp1;
          temp1->next = temp3;
          swap_count++;
          printf("Changed equal))) %d\n", swap_count);
        }

        pthread_rwlock_unlock(&temp0->sync);
        pthread_rwlock_unlock(&temp1->sync);
        pthread_rwlock_unlock(&temp2->sync);
        break;
      }
      current = current->next;
      n++;
    } while (current != NULL && current->next != NULL &&
             current->next->next != NULL);
  }
}

// Переписанные функции, использующие блокировки чтения-записи
// (например, findEqualLengthStrings и swapNodesIncrease)...

int main() {
  Storage storage;
  initStorage(&storage, VALUES);
  pthread_t thread1, thread2, thread3, thread21, thread22, thread23;

  pthread_create(&thread1, NULL, findIncreasingLengthStrings, (void *)&storage);
  pthread_create(&thread2, NULL, findDecreasingLengthStrings, (void *)&storage);
  pthread_create(&thread3, NULL, findEqualLengthStrings, (void *)&storage);

  pthread_create(&thread21, NULL, swapNodesIncrease, (void *)&storage);
  pthread_create(&thread22, NULL, swapNodesDecrease, (void *)&storage);
  pthread_create(&thread23, NULL, swapNodesEqual, (void *)&storage);

  // Создание потоков и их запуск...

  pthread_join(thread21, NULL);
  pthread_join(thread22, NULL);
  pthread_join(thread23, NULL);
  pthread_join(thread3, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread1, NULL);

  freeStorage(&storage);

  return 0;
}
