

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>
#define STACK_SIZE 4096
#define NAME_SIZE 16
#define MAX_THREADS 8

typedef struct uthread {
  char name[NAME_SIZE];
  void (*thread_func)(void *);
  void *arg;
  ucontext_t uctx;
} uthread_t;

uthread_t *uthreads[MAX_THREADS];
int uthread_count = 0;
int uthread_cur = 0;

void *create_stack(off_t size, char *file_name) {
  void *stack;
  if (file_name) {
    int stack_fd;
    stack_fd = open(file_name, O_RDWR | O_CREAT, 0660);
    ftruncate(stack_fd, 0);
    ftruncate(stack_fd, size);
    stack = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_STACK,
                 stack_fd, 0);
    close(stack_fd);
  } else {
    stack = mmap(NULL, size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  }
  memset(stack, 0x7f, size);
  return stack;
}

void sheduler(void) {
  int err;
  ucontext_t *cur_ctx, *next_ctx;
  cur_ctx = &(uthreads[uthread_cur]->uctx);
  uthread_cur = (uthread_cur + 1) % uthread_count;
  next_ctx = &(uthreads[uthread_cur]->uctx);

  err = swapcontext(cur_ctx, next_ctx);
  if (err == -1) {
    printf("sheduler: swapcontext() failed: %s\n", strerror(errno));
    exit(1);
  }
}

void start_thread(void) {
  int i;
  ucontext_t *ctx;
  for (i = 1; i < uthread_count; i++) {
    ctx = &uthreads[i]->uctx;
    char *stack_from = ctx->uc_stack.ss_sp;
    char *stack_to = ctx->uc_stack.ss_sp + ctx->uc_stack.ss_size;
    // printf("start_thread: %p stack: %p-%p\n", &i, stack_from, stack_to);
    if (stack_from <= (char *)&i && (char *)&i <= stack_to) {
      printf("start_thread: i=%d name: '%s' thread_func: %p; arg: %p\n", i,
             uthreads[i]->name, uthreads[i]->thread_func, uthreads[i]->arg);
      uthreads[i]->thread_func(uthreads[i]->arg);
    }
  }
}

void uthread_create(uthread_t **ut, char *name, void (*thread_func)(void *),
                    void *arg) {
  char *stack;
  uthread_t *new_ut;
  int err;
  stack = create_stack(STACK_SIZE, name);
  new_ut = (uthread_t *)(stack + STACK_SIZE - sizeof(uthread_t));
  err = getcontext(&new_ut->uctx);
  if (err == -1) {
    printf("uthread_create: getcontext() failed: %s\n", strerror(errno));
    exit(2);
  }

  new_ut->uctx.uc_stack.ss_sp = stack;
  new_ut->uctx.uc_stack.ss_size = STACK_SIZE - sizeof(uthread_t);
  new_ut->uctx.uc_link = NULL;
  makecontext(&new_ut->uctx, start_thread, 0);
  new_ut->thread_func = thread_func;
  new_ut->arg = arg;
  strncpy(new_ut->name, name, NAME_SIZE);
  uthreads[uthread_count] = new_ut;
  uthread_count++;
  *ut = new_ut;
}

void uthread_scheduler(int sig, siginfo_t *si, void *u) {
  printf("uthread_scheduler: \n");
  alarm(1);
  sheduler();
}

void mythread1(void *arg) {
  int i;
  char *myarg = (char *)arg;
  printf("mythread: started: arg '%s', %p %d %d %d\n", myarg, &i, getpid(),
         getppid(), gettid());
  for (i = 0; i < 10000; i++) {
    printf("mythread: arg '%s' %p sheduler()\n", myarg, &i);
    sleep(1);
  }
  printf("mythread: finished\n");
}

int main(int argc, char *argv[]) {
  uthread_t *ut[3];
  char *arg[] = {"thread 1", "thread 2", "thread 3"};
  struct sigaction act;

  uthread_t main_thread;
  uthreads[0] = &main_thread;
  uthread_count = 1;

  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_ONSTACK | SA_SIGINFO;
  act.sa_sigaction = uthread_scheduler;
  // sigaction(SIGUSR1, &act, NULL);

  sigaction(SIGALRM, &act, NULL);
  alarm(1);

  printf("main: started: %d %d %d\n", getpid(), getppid(), gettid());
  uthread_create(&ut[0], "user thread 1", mythread1, (void *)arg[0]);
  uthread_create(&ut[1], "user thread 2", mythread1, (void *)arg[1]);
  uthread_create(&ut[2], "user thread 3", mythread1, (void *)arg[2]);
  while (1) {
    sheduler();
    printf("main: while: %d %d %d\n", getpid(), getppid(), gettid());
    // sleep(1);
  }
  printf("main: finished: %d %d %d\n", getpid(), getppid(), gettid());
  return 0;
}