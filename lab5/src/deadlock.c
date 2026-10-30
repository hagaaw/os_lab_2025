/*
 * deadlock.c - демонстрация взаимной блокировки (deadlock).
 *
 * Сборка: gcc deadlock.c -o deadlock -pthread
 * Запуск: ./deadlock         - программа зависнет (deadlock)
 *         ./deadlock --fix   - одинаковый порядок захвата, завершится нормально
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
 
pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;
int fix = 0;
 
void *thread1(void *arg) {
  (void)arg;
  pthread_mutex_lock(&mutex_a);
  printf("Поток 1: захватил A, жду B\n");
  sleep(1); /* даём второму потоку захватить B */
  pthread_mutex_lock(&mutex_b);
  printf("Поток 1: захватил A и B\n");
  pthread_mutex_unlock(&mutex_b);
  pthread_mutex_unlock(&mutex_a);
  return NULL;
}
 
void *thread2(void *arg) {
  (void)arg;
  pthread_mutex_t *first = fix ? &mutex_a : &mutex_b;
  pthread_mutex_t *second = fix ? &mutex_b : &mutex_a;
  pthread_mutex_lock(first);
  printf("Поток 2: захватил %s, жду %s\n", fix ? "A" : "B", fix ? "B" : "A");
  sleep(1);
  pthread_mutex_lock(second);
  printf("Поток 2: захватил A и B\n");
  pthread_mutex_unlock(second);
  pthread_mutex_unlock(first);
  return NULL;
}
 
int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0); /* вывод сразу, даже если процесс зависнет */
  if (argc > 1 && strcmp(argv[1], "--fix") == 0) fix = 1;
  pthread_t t1, t2;
  pthread_create(&t1, NULL, thread1, NULL);
  pthread_create(&t2, NULL, thread2, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  printf("Программа завершилась без deadlock\n");
  return 0;
}
