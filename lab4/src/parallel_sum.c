#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/time.h>

#include "sum_lib.h"
#include "utils.h"

/* Обёртка для потока: вызывает Sum и возвращает результат как void* */
void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  int s = Sum(sum_args);
  return (void *)(intptr_t)s;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  /* --- Разбор аргументов --- */
  while (1) {
    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"seed",        required_argument, 0, 0},
        {"array_size",  required_argument, 0, 0},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0: threads_num = (uint32_t)atoi(optarg); break;
          case 1: seed        = (uint32_t)atoi(optarg); break;
          case 2: array_size  = (uint32_t)atoi(optarg); break;
          default:
            printf("Индекс %d вне диапазона опций\n", option_index);
            return 1;
        }
        break;
      case '?':
        return 1;
    }
  }

  if (threads_num == 0 || array_size == 0) {
    printf("Использование: %s --threads_num \"num\" --seed \"num\" "
           "--array_size \"num\"\n", argv[0]);
    return 1;
  }
  if (threads_num > array_size) {
    printf("threads_num не может быть больше array_size\n");
    return 1;
  }

  /* --- Генерация массива (НЕ входит в замер) --- */
  int *array = malloc(sizeof(int) * array_size);
  if (!array) {
    printf("Ошибка malloc\n");
    return 1;
  }
  GenerateArray(array, array_size, (int)seed);

  /* --- Подготовка аргументов для потоков --- */
  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = (int)(i * array_size / threads_num);
    args[i].end   = (int)((i + 1) * array_size / threads_num);
  }

  /* --- Старт замера --- */
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  /* --- Запуск потоков --- */
  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]) != 0) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }

  /* --- Сбор результатов --- */
  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    intptr_t partial = 0;
    pthread_join(threads[i], (void **)&partial);
    total_sum += (int)partial;
  }

  /* --- Конец замера --- */
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %f ms\n", elapsed_time);
  return 0;
}