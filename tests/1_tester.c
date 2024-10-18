#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  if (fork() == 0) {
    execve("./bin/1_init", NULL, NULL);

    perror("execve");
    exit(EXIT_FAILURE);
  }
  wait(0);

  printf("Testing started...\n");
  if (fork() == 0) {
    execve("./bin/1_main", NULL, NULL);

    perror("execve");
    exit(EXIT_FAILURE);
  }
  wait(0);

  printf("Testing ended!\n");

  execve("./bin/1_print", NULL, NULL);
  perror("execve");
  exit(EXIT_FAILURE);
}
