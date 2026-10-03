#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  int seed = atoi(argv[1]);
  if (seed <= 0) {
    printf("seed is a positive number\n");
    return 1;
  }

  int array_size = atoi(argv[2]);
  if (array_size <= 0) {
    printf("array_size is a positive number\n");
    return 1;
  }

  pid_t child_pid = fork();

  if (child_pid == -1) {
    perror("fork");
    return 1;
  }

  if (child_pid == 0) {
    char *child_argv[] = {
      "./sequential_min_max",
      argv[1],
      argv[2],
      NULL
    };
    execv("./sequential_min_max", child_argv);

    perror("execv");
    return 1;
  }

  int status;
  waitpid(child_pid, &status, 0);

  if (WIFEXITED(status)) {
    printf("Child exited with code %d\n", WEXITSTATUS(status));
  } else {
    printf("Child terminated abnormally\n");
  }

  return 0;
}