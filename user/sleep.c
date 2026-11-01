#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  if(argc != 2){
    fprintf(2, "usage: sleep ticks\n");
    exit(1);
  }
  for(char *p = argv[1]; *p; p++)
    if(*p < '0' || *p > '9'){
      fprintf(2, "sleep: invalid ticks\n");
      exit(1);
    }
  if(argv[1][0] == 0 || sleep(atoi(argv[1])) < 0)
    exit(1);
  exit(0);
}
