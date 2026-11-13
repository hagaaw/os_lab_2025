#define _GNU_SOURCE
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

struct FactorialArgs {
  uint64_t begin, end, mod;
  uint64_t result;
};

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i <= args->end; i++)
    ans = MultModulo(ans, i, args->mod);
  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  fargs->result = Factorial(fargs);
  return NULL;
}

int main(int argc, char **argv) {
  int tnum = -1, port = -1;

  while (true) {
    static struct option options[] = {
        {"port", required_argument, 0, 0},
        {"tnum", required_argument, 0, 0},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    if (c == 0) {
      if (option_index == 0) port = atoi(optarg);
      else if (option_index == 1) tnum = atoi(optarg);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) { perror("socket"); return 1; }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("bind"); return 1;
  }
  if (listen(server_fd, 128) < 0) { perror("listen"); return 1; }

  printf("Server listening at %d\n", port);
  fflush(stdout);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) continue;

    while (true) {
      char from_client[sizeof(uint64_t) * 3];
      int r = recv(client_fd, from_client, sizeof(from_client), MSG_WAITALL);
      if (r <= 0) break;

      uint64_t begin, end, mod;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end,   from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod,   from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      printf("Receive: %llu %llu %llu\n",
             (unsigned long long)begin,
             (unsigned long long)end,
             (unsigned long long)mod);
      fflush(stdout);

      /* --- разбиваем [begin..end] на tnum потоков --- */
      pthread_t threads[tnum];
      struct FactorialArgs args[tnum];
      uint64_t len = (end >= begin) ? (end - begin + 1) : 0;
      uint64_t chunk = (len + tnum - 1) / tnum;
      if (chunk == 0) chunk = 1;

      for (int i = 0; i < tnum; i++) {
        args[i].begin = begin + (uint64_t)i * chunk;
        args[i].end   = args[i].begin + chunk - 1;
        if (args[i].begin > end) args[i].begin = end + 1; /* пусто */
        if (args[i].end > end)   args[i].end = end;
        args[i].mod   = mod;
        args[i].result = 1;
        pthread_create(&threads[i], NULL, ThreadFactorial, &args[i]);
      }

      uint64_t total = 1;
      for (int i = 0; i < tnum; i++) {
        pthread_join(threads[i], NULL);
        total = MultModulo(total, args[i].result, mod);
      }

      printf("Total: %llu\n", (unsigned long long)total);
      fflush(stdout);
      send(client_fd, &total, sizeof(total), 0);
    }

    close(client_fd);
  }
  return 0;
}