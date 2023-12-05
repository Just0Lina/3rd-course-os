#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define MAX_THREADS 10

typedef struct {
  ucontext_t context;
  void *(*start_routine)(void *);
  void *arg;
} uthread_t;

#define handle_error(msg) \
  do {                    \
    perror(msg);          \
    exit(EXIT_FAILURE);   \
  } while (0)

uthread_t threads[MAX_THREADS];
int current_thread = 0;
static ucontext_t uctx_main;

// void uthread_exit() {
//   // Выводим сообщение перед завершением потока
//   printf("Thread %d is done\n", current_thread);

//   // Завершаем поток
//   threads[current_thread].start_routine(threads[current_thread].arg);
//   current_thread = -1;

//   // Переключаемся к другому потоку (упрощенно)
//   current_thread = (current_thread + 1) % MAX_THREADS;
//   setcontext(&threads[current_thread].context);
// }

int uthread_create(uthread_t *thread, void *(*start_routine)(void *),
                   void *arg) {
  if (current_thread >= MAX_THREADS) {
    return -1;  // Максимальное количество потоков достигнуто
  }

  thread->start_routine = start_routine;
  thread->arg = arg;

  if (getcontext(&thread->context) == -1) {
    perror("getcontext");
    return -1;
  }

  char *stack = malloc(sizeof(char) * 16384);  // Размер стека
  thread->context.uc_stack.ss_sp = stack;
  thread->context.uc_stack.ss_size = 16384;
  thread->context.uc_link = &uctx_main;

  makecontext(&thread->context, (void (*)())start_routine, 1, arg);

  return 0;
}

void *thread_function(void *arg) {
  int thread_id = *((int *)arg);

  printf("Thread %d is started\n", thread_id);

  for (int i = 0; i < thread_id + 1; ++i) printf("meow ");
  printf("\nThread %d is finished\n\n", thread_id);

  return NULL;
}

int main() {
  printf("sizeof(ucontext) = %ld\n", sizeof(ucontext_t));
  int thread_id[3];
  // Создаем несколько пользовательских потоков
  for (int i = 0; i < 3; i++) {
    thread_id[i] = i;
  }
  for (int i = 0; i < 3; i++) {
    uthread_t thread;
    if (uthread_create(&thread, thread_function, &thread_id[i]) == -1) {
      fprintf(stderr, "Error creating thread\n");
      exit(EXIT_FAILURE);
    }
    threads[i] = thread;
  }

  for (int i = 0; i < 3; i++) {
    int thread_id = i;
    uthread_t thread;
    if (uthread_create(&thread, thread_function, &thread_id) == -1) {
      fprintf(stderr, "Error creating thread\n");
      exit(EXIT_FAILURE);
    }
    threads[i] = thread;
    printf("main: swapcontext(&uctx_main, &threads[%d].context)\n", i);
    if (swapcontext(&uctx_main, &threads[i].context) == -1)
      handle_error("swapcontext");  // Explicit context switch
  }
  return 0;
}
