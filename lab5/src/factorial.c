/*
 * factorial.c - параллельное вычисление k! по модулю mod.
 *
 * Сборка: gcc factorial.c -o factorial -pthread
 * Запуск: ./factorial -k 10 --pnum=4 --mod=10
 */
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
 
typedef unsigned long long u64;
 
static u64 result = 1;  /* общий результат */
static u64 g_mod = 1;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
 
struct range {
  u64 from; /* включительно */
  u64 to;   /* включительно */
};
 
static void *worker(void *arg) {
  struct range *r = (struct range *)arg;
  u64 part = 1 % g_mod;
  for (u64 i = r->from; i <= r->to; i++)
    part = (u64)((__uint128_t)part * (i % g_mod) % g_mod);
 
  /* критическая секция: объединяем частичный результат с общим */
  pthread_mutex_lock(&mut);
  result = (u64)((__uint128_t)result * part % g_mod);
  pthread_mutex_unlock(&mut);
  return NULL;
}
 
static void usage(const char *prog) {
  fprintf(stderr, "Usage: %s -k <num> --pnum=<threads> --mod=<modulus>\n", prog);
}
 
int main(int argc, char **argv) {
  long long k = -1, pnum = -1, mod = -1;
  static struct option opts[] = {{"pnum", required_argument, 0, 'p'},
                                 {"mod", required_argument, 0, 'm'},
                                 {"k", required_argument, 0, 'k'},
                                 {0, 0, 0, 0}};
  int c;
  while ((c = getopt_long(argc, argv, "k:", opts, NULL)) != -1) {
    switch (c) {
    case 'k': k = atoll(optarg); break;
    case 'p': pnum = atoll(optarg); break;
    case 'm': mod = atoll(optarg); break;
    default: usage(argv[0]); return 1;
    }
  }
  if (k < 0 || pnum <= 0 || mod <= 0) {
    usage(argv[0]);
    return 1;
  }
  g_mod = (u64)mod;
  result = 1 % g_mod;
 
  if (pnum > k && k > 0) pnum = k; /* потоков не больше, чем чисел */
  if (k == 0) pnum = 1;
 
  pthread_t *threads = malloc(sizeof(pthread_t) * pnum);
  struct range *ranges = malloc(sizeof(struct range) * pnum);
  if (!threads || !ranges) {
    perror("malloc");
    return 1;
  }
 
  /* делим числа 1..k на pnum непрерывных кусков */
  u64 base = (u64)k / pnum, extra = (u64)k % pnum, cur = 1;
  for (long long i = 0; i < pnum; i++) {
    u64 len = base + ((u64)i < extra ? 1 : 0);
    ranges[i].from = cur;
    ranges[i].to = cur + len - 1; /* при len == 0 диапазон пуст */
    cur += len;
    if (pthread_create(&threads[i], NULL, worker, &ranges[i]) != 0) {
      perror("pthread_create");
      return 1;
    }
  }
  for (long long i = 0; i < pnum; i++)
    pthread_join(threads[i], NULL);
 
  printf("%lld! mod %lld = %llu\n", k, mod, result);
  free(threads);
  free(ranges);
  return 0;
}
