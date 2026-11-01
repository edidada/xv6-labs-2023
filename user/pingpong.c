#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int down[2], up[2];
  char byte = 'p';
  if(pipe(down) < 0 || pipe(up) < 0)
    exit(1);
  int pid = fork();
  if(pid < 0)
    exit(1);
  if(pid == 0){
    close(down[1]);
    close(up[0]);
    if(read(down[0], &byte, 1) != 1)
      exit(1);
    printf("%d: received ping\n", getpid());
    if(write(up[1], &byte, 1) != 1)
      exit(1);
    close(down[0]);
    close(up[1]);
    exit(0);
  }
  close(down[0]);
  close(up[1]);
  if(write(down[1], &byte, 1) != 1 || read(up[0], &byte, 1) != 1)
    exit(1);
  printf("%d: received pong\n", getpid());
  close(down[1]);
  close(up[0]);
  int status;
  wait(&status);
  exit(status);
}
