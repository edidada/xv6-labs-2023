#include "kernel/types.h"
#include "user/user.h"

extern int abisum(int);

int
main(void)
{
  for(int n = 0; n < 100; n++){
    if(abisum(n) != n * (n + 1)){
      fprintf(2, "ABI test failed at %d\n", n);
      exit(1);
    }
  }
  printf("ABI TESTS PASSED\n");
  exit(0);
}
