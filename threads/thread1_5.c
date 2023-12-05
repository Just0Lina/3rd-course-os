#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum { COUNT_THREAD = 4 };

#define handle_error_en(en, msg) \
  do {                           \
    errno = en;                  \
    perror(msg);                 \
    exit(EXIT_FAILURE);          \
  } while (0)

void signal_handler(int sig_num) {
  printf("Caught signal %d, tid %d\n", sig_num, gettid());
}

void *block_all_signal() {
  printf("call block_all_signal %d\n", gettid());
  sigset_t set_all_signal;

  int err = sigfillset(&set_all_signal);
  if (err) handle_error_en(err, "sigfillset");

  err = pthread_sigmask(SIG_BLOCK, &set_all_signal, NULL);
  if (err) handle_error_en(err, "pthread_sigmask");

  sleep(2);
  return NULL;
}

// https://habr.com/ru/articles/141206/

void *block_signals(void *arg) {
  printf("block_signals [%d]: Hello from mythread!\n", gettid());
  sigset_t set;
  sigfillset(&set);  // Блокировать все сигналы

  // Установить блокировку для текущего потока
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (1) {
    sleep(1);
  }
  return NULL;
}

void sigint_handler(int sig) { printf("Received SIGINT\n"); }

void *handle_sigint(void *arg) {
  printf("handle_sigint [%d]: Hello from mythread!\n", gettid());

  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);

  while (1) {
    sleep(1);
  }
  return NULL;
}

void *handle_sigquit(void *arg) {
  printf("handle_sigquit [%d]: Hello from mythread!\n", gettid());

  int sig;
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGQUIT);
  sigaddset(&set, SIGINT);

  sigwait(&set, &sig);
  printf("Received SIGQUIT\n");

  while (1) {
    sleep(1);
  }
  return NULL;
}

int main() {
  pthread_t block_thread;  // sigint_thread, sigquit_thread;
  // Создаем поток для блокировки всех сигналов
  sigset_t all_signals;
  sigfillset(&all_signals);
  pthread_sigmask(SIG_BLOCK, &all_signals, NULL);
  printf("mythread [%d]: Hello from mythread!\n", getpid());
  // pthread_create(&block_thread, NULL, block_signals, NULL);
  // pthread_create(&sigint_thread, NULL, handle_sigint, NULL);
  // pthread_create(&sigquit_thread, NULL, handle_sigquit, NULL);

  // pthread_join(block_thread, NULL);
  // pthread_join(sigint_thread, NULL);
  // pthread_join(sigquit_thread, NULL);

  pthread_create(&block_thread, NULL, block_signals, NULL);

  while (true) {
    pthread_kill(block_thread, SIGINT);
    pthread_kill(block_thread, SIGQUIT);
  }
  q->get_attempts++;

  pause();
  return 0;
}
