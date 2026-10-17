#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include <signal.h>
#include <errno.h>

#include "find_min_max.h"
#include "utils.h"

/* ---- глобальные переменные для обработчика SIGALRM ---- */
static pid_t child_pids[1024];                  /* PID дочерних процессов */
static volatile sig_atomic_t child_count = 0;   /* сколько PID запомнили */
static volatile sig_atomic_t timeout_fired = 0; /* сработал ли таймаут */

/* Обработчик сигнала SIGALRM: убивает всех детей SIGKILL'ом */
void on_alarm(int sig) {
  (void)sig;
  timeout_fired = 1;
  for (int i = 0; i < child_count; i++) {
    if (child_pids[i] > 0) {
      kill(child_pids[i], SIGKILL);
    }
  }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;              /* -1 = таймаут не задан */
  bool with_files = false;

  /* Разбор аргументов командной строки */
  while (true) {
    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            break;
          case 1:
            array_size = atoi(optarg);
            break;
          case 2:
            pnum = atoi(optarg);
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            break;
          default:
            printf("Индекс %d вне диапазона опций\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt вернул код символа 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Есть хотя бы один аргумент без опции\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Использование: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
           "[--by_files] [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  /* Проверка, что pnum не превышает размер массива child_pids */
  if (pnum > (int)(sizeof(child_pids) / sizeof(child_pids[0]))) {
    printf("pnum слишком велик (максимум %zu)\n",
           sizeof(child_pids) / sizeof(child_pids[0]));
    return 1;
  }

  /* Генерация массива случайных чисел */
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  /* Трубы: на каждый процесс по 2 (для min и max) */
  int pipes[2 * pnum][2];

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[2 * i]) == -1 || pipe(pipes[2 * i + 1]) == -1) {
        perror("pipe");
        return 1;
      }
    }
  }

  /* ---- Установка обработчика SIGALRM и запуск таймера ---- */
  if (timeout > 0) {
    signal(SIGALRM, on_alarm);
    alarm(timeout);
  }

  /* Создание дочерних процессов */
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();

    if (child_pid == 0) {
      /* === РЕБЁНОК === */

      /* Считаем min/max на своём куске массива */
      int start = i * array_size / pnum;
      int end = (i + 1) * array_size / pnum;
      struct MinMax mm = GetMinMax(array, start, end);

      if (with_files) {
        /* Пишем результаты в файлы */
        char filename[64];
        FILE *f;

        sprintf(filename, "min_%d.txt", i);
        f = fopen(filename, "w");
        if (f) {
          fprintf(f, "%d", mm.min);
          fclose(f);
        }

        sprintf(filename, "max_%d.txt", i);
        f = fopen(filename, "w");
        if (f) {
          fprintf(f, "%d", mm.max);
          fclose(f);
        }
      } else {
        /* Отправляем результаты через трубы */
        close(pipes[2 * i][0]);      /* закрываем чтение min */
        close(pipes[2 * i + 1][0]);  /* закрываем чтение max */
        write(pipes[2 * i][1], &mm.min, sizeof(int));
        write(pipes[2 * i + 1][1], &mm.max, sizeof(int));
        close(pipes[2 * i][1]);
        close(pipes[2 * i + 1][1]);
      }

      free(array);
      return 0;
    } else if (child_pid > 0) {
      /* === РОДИТЕЛЬ === */
      active_child_processes += 1;

      /* Запоминаем PID, чтобы обработчик SIGALRM мог убить ребёнка */
      child_pids[child_count] = child_pid;
      child_count += 1;

      if (!with_files) {
        /* В родителе закрываем концы труб на запись */
        close(pipes[2 * i][1]);
        close(pipes[2 * i + 1][1]);
      }
    } else {
      printf("Ошибка fork!\n");
      return 1;
    }
  }

  /* ---- Неблокирующее ожидание детей (WNOHANG) ---- */
  while (active_child_processes > 0) {
    int status;
    pid_t w = waitpid(-1, &status, WNOHANG);

    if (w > 0) {
      /* Один ребёнок завершился (или был убит) */
      active_child_processes -= 1;
    } else if (w == 0) {
      /* Дети ещё живы, но никто не готов — короткая пауза */
      usleep(1000); /* 1 мс */
    } else {
      /* w == -1 */
      if (errno == EINTR) continue; /* прервано сигналом SIGALRM */
      break;                        /* ECHILD и прочее */
    }
  }

  /* Отменяем таймер, если всё успели */
  if (timeout > 0) {
    alarm(0);
  }

  /* Сбор и агрегация результатов от всех детей */
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[64];
      FILE *f;
      bool ok_min = false, ok_max = false;

      sprintf(filename, "min_%d.txt", i);
      f = fopen(filename, "r");
      if (f) {
        if (fscanf(f, "%d", &min) == 1) ok_min = true;
        fclose(f);
      }
      remove(filename);

      sprintf(filename, "max_%d.txt", i);
      f = fopen(filename, "r");
      if (f) {
        if (fscanf(f, "%d", &max) == 1) ok_max = true;
        fclose(f);
      }
      remove(filename);

      if (!ok_min || !ok_max) {
        /* Ребёнок не успел / был убит — пропускаем его результаты */
        continue;
      }
    } else {
      ssize_t r1 = read(pipes[2 * i][0], &min, sizeof(int));
      ssize_t r2 = read(pipes[2 * i + 1][0], &max, sizeof(int));
      close(pipes[2 * i][0]);
      close(pipes[2 * i + 1][0]);

      if (r1 != sizeof(int) || r2 != sizeof(int)) {
        /* Ребёнок не успел / был убит — пропускаем его результаты */
        continue;
      }
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  /* Замер времени выполнения */
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  /* Вывод результатов */
  if (min_max.min == INT_MAX || min_max.max == INT_MIN) {
    /* Все дети были убиты — данных нет */
    printf("Мин: Н/Д\n");
    printf("Макс: Н/Д\n");
  } else {
    printf("Мин: %d\n", min_max.min);
    printf("Макс: %d\n", min_max.max);
  }
  if (timeout_fired) {
    printf("Таймаут сработал: дочерние процессы убиты сигналом SIGKILL\n");
  }
  printf("Затраченное время: %f мс\n", elapsed_time);
  fflush(NULL);
  return 0;
}