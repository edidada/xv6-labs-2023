#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

static int
run(char **args, int fixed, char *line)
{
  int n = fixed;
  char *p = line;
  while(*p){
    while(*p == ' ' || *p == '\t')
      p++;
    if(!*p)
      break;
    if(n >= MAXARG - 1){
      fprintf(2, "xargs: too many arguments\n");
      return 1;
    }
    args[n++] = p;
    while(*p && *p != ' ' && *p != '\t')
      p++;
    if(*p)
      *p++ = 0;
  }
  args[n] = 0;
  int pid = fork();
  if(pid < 0)
    return 1;
  if(pid == 0){
    exec(args[0], args);
    fprintf(2, "xargs: exec %s failed\n", args[0]);
    exit(1);
  }
  int status;
  if(wait(&status) < 0)
    return 1;
  return status;
}

int
main(int argc, char **argv)
{
  char *args[MAXARG], line[512], c;
  int used = 0, failed = 0, count;
  if(argc < 2 || argc > MAXARG){
    fprintf(2, "usage: xargs command [arguments...]\n");
    exit(1);
  }
  for(int i = 1; i < argc; i++)
    args[i - 1] = argv[i];
  while((count = read(0, &c, 1)) == 1){
    if(c == '\n'){
      line[used] = 0;
      failed |= run(args, argc - 1, line);
      used = 0;
    } else {
      if(used == sizeof line - 1){
        fprintf(2, "xargs: line too long\n");
        exit(1);
      }
      line[used++] = c;
    }
  }
  if(used){
    line[used] = 0;
    failed |= run(args, argc - 1, line);
  }
  exit(count < 0 ? 1 : failed);
}
