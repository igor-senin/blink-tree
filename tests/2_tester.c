#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  if (fork() == 0) {
    execve("./2_init", NULL, NULL);

    perror("execve");
    exit(EXIT_FAILURE);
  }
  wait(0);

  printf("Testing started...\n");
  for (int i = 0; i < 16; ++i) {
    if (fork() == 0) {
      execve("./2_main", NULL, NULL);

      perror("execve");
      exit(EXIT_FAILURE);
    }
  }
  wait(0);

  printf("Testing ended!\n");

  execve("./2_print", NULL, NULL);
  perror("execve");
  exit(EXIT_FAILURE);
}
