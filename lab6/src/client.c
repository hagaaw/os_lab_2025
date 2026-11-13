#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

struct Server {
  char ip[255];
  int port;
};

/* Одна задача = один сервер + диапазон + место под ответ */
struct Task {
  struct Server srv;
  uint64_t begin, end, mod;
  uint64_t result;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE || end == str) {
    fprintf(stderr, "Bad value: %s\n", str);
    return false;
  }
  *val = i;
  return true;
}

/* Тело потока: connect -> send -> recv -> close */
void *Worker(void *arg) {
  struct Task *t = (struct Task *)arg;

  struct hostent *hostname = gethostbyname(t->srv.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed: %s\n", t->srv.ip);
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(t->srv.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) { perror("socket"); return NULL; }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("connect");
    close(sck);
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &t->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &t->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &t->mod, sizeof(uint64_t));
  send(sck, task, sizeof(task), 0);

  recv(sck, &t->result, sizeof(t->result), MSG_WAITALL);
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1, mod = -1;
  char servers[255] = {0};

  while (true) {
    static struct option options[] = {
        {"k", required_argument, 0, 0},
        {"mod", required_argument, 0, 0},
        {"servers", required_argument, 0, 0},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    if (c == 0) {
      switch (option_index) {
      case 0: ConvertStringToUI64(optarg, &k); break;
      case 1: ConvertStringToUI64(optarg, &mod); break;
      case 2: snprintf(servers, sizeof(servers), "%s", optarg); break;
      }
    }
  }

  if (k == (uint64_t)-1 || mod == (uint64_t)-1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers file\n", argv[0]);
    return 1;
  }

  /* --- читаем серверы из файла --- */
  FILE *f = fopen(servers, "r");
  if (!f) { perror("fopen"); return 1; }

  struct Server list[64];
  int servers_num = 0;
  char line[300];
  while (fgets(line, sizeof(line), f) && servers_num < 64) {
    line[strcspn(line, "\r\n")] = '\0';
    char *colon = strrchr(line, ':');
    if (!colon) continue;
    *colon = '\0';
    snprintf(list[servers_num].ip, sizeof(list[servers_num].ip), "%s", line);
    list[servers_num].port = atoi(colon + 1);
    servers_num++;
  }
  fclose(f);
  if (servers_num == 0) { fprintf(stderr, "no servers\n"); return 1; }

  /* --- разбиваем [1..k] на servers_num примерно равных кусков --- */
  struct Task tasks[64];
  pthread_t threads[64];
  uint64_t chunk = (k + servers_num - 1) / servers_num;

  for (int i = 0; i < servers_num; i++) {
    tasks[i].srv   = list[i];
    tasks[i].begin = (uint64_t)i * chunk + 1;
    tasks[i].end   = tasks[i].begin + chunk - 1;
    if (tasks[i].end > k) tasks[i].end = k;
    tasks[i].mod   = mod;
    tasks[i].result = 1;
  }

  /* --- запускаем все потоки сразу --- */
  for (int i = 0; i < servers_num; i++)
    pthread_create(&threads[i], NULL, Worker, &tasks[i]);

  /* --- собираем результаты --- */
  uint64_t answer = 1;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    answer = MultModulo(answer, tasks[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)answer);
  return 0;
}