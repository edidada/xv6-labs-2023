#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int input[2];
  if(pipe(input) < 0)
    exit(1);
  int pid = fork();
  if(pid < 0)
    exit(1);
  if(pid > 0){
    close(input[0]);
    for(int n = 2; n <= 35; n++)
      if(write(input[1], &n, sizeof n) != sizeof n)
        exit(1);
    close(input[1]);
    int status;
    wait(&status);
    exit(status);
  }
  close(input[1]);
  for(;;){
    int prime, n, next[2];
    int count = read(input[0], &prime, sizeof prime);
    if(count == 0){
      close(input[0]);
      exit(0);
    }
    if(count != sizeof prime || pipe(next) < 0)
      exit(1);
    printf("prime %d\n", prime);
    pid = fork();
    if(pid < 0)
      exit(1);
    if(pid == 0){
      close(input[0]);
      close(next[1]);
      input[0] = next[0];
      continue;
    }
    close(next[0]);
    while((count = read(input[0], &n, sizeof n)) == sizeof n)
      if(n % prime && write(next[1], &n, sizeof n) != sizeof n)
        exit(1);
    close(input[0]);
    close(next[1]);
    int status;
    wait(&status);
    exit(count < 0 ? 1 : status);
  }
}
