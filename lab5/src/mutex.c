/*
 * mutex.c - два потока увеличивают общую переменную common.
 *
 * Сборка без мьютекса:  gcc mutex.c -o mutex_nolock -pthread
 * Сборка с мьютексом:   gcc -DUSE_MUTEX mutex.c -o mutex_lock -pthread
 */
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
 
void *do_one_thing(void *);
void *do_another_thing(void *);
void do_wrap_up(int);
 
int common = 0; /* общая переменная для двух потоков */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
 
#ifdef USE_MUTEX
#define LOCK() pthread_mutex_lock(&mut)
#define UNLOCK() pthread_mutex_unlock(&mut)
#else
#define LOCK()
#define UNLOCK()
#endif
 
int main() {
  pthread_t thread1, thread2;
 
  if (pthread_create(&thread1, NULL, do_one_thing, (void *)&common) != 0) {
    perror("pthread_create");
    exit(1);
  }
  if (pthread_create(&thread2, NULL, do_another_thing, (void *)&common) != 0) {
    perror("pthread_create");
    exit(1);
  }
  if (pthread_join(thread1, NULL) != 0) {
    perror("pthread_join");
    exit(1);
  }
  if (pthread_join(thread2, NULL) != 0) {
    perror("pthread_join");
    exit(1);
  }
 
  do_wrap_up(common);
  return 0;
}
 
void *do_one_thing(void *arg) {
  int *pnum_times = (int *)arg;
  int i, work;
  unsigned long k;
  for (i = 0; i < 50; i++) {
    LOCK();
    printf("doing one thing\n");
    work = *pnum_times;
    printf("counter = %d\n", work);
    work++; /* увеличиваем, но пока не записываем */
    for (k = 0; k < 500000; k++)
      ;                 /* долгий цикл */
    *pnum_times = work; /* записываем обратно */
    UNLOCK();
  }
  return NULL;
}
 
void *do_another_thing(void *arg) {
  int *pnum_times = (int *)arg;
  int i, work;
  unsigned long k;
  for (i = 0; i < 50; i++) {
    LOCK();
    printf("doing another thing\n");
    work = *pnum_times;
    printf("counter = %d\n", work);
    work++;
    for (k = 0; k < 500000; k++)
      ;
    *pnum_times = work;
    UNLOCK();
  }
  return NULL;
}
 
void do_wrap_up(int counter) {
  printf("All done, counter = %d\n", counter);
}
