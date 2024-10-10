#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>




#define KB 1024
#define MEMORYPOOL_SIZE 4*KB
#define STRIDE 4
#define NUM_ITERATIONS 1


register int* mempool asm ("r12");

unsigned __attribute__ ((noinline)) SIM_BEGIN(bool i)
{

   if (i==false) return 0;
   printf("sim begin\n");
   return 1;
}

unsigned __attribute__ ((noinline)) SIM_END(bool i)
{

   if (i==false) return 0;
   printf("sim end\n");
   return 1;

} 


int64_t read_bench () {

  register int64_t sum=0;
  register int i=0;
  SIM_BEGIN(1);
  for (i = 0; i < MEMORYPOOL_SIZE/4; i+=STRIDE/4) {
    sum+=mempool[i];
    
  }
  SIM_END(1);

 return sum;

}
int main() {

  // allocate MEMORYPPOL_SIZE BYTES
  mempool = (int*) malloc(MEMORYPOOL_SIZE);

  // check if memory has been allocated successfully
  if (!mempool) {
    printf("Memory Allocation Failed");
    exit(1);
  }

  


  for (int i = 0; i < MEMORYPOOL_SIZE/4; i+=STRIDE/4) {
    mempool[i] = i;
    printf("%p\n",&mempool[i]);
  }
  int64_t sum=0;
  
  
  for (int j =0; j<NUM_ITERATIONS;j++)
      sum+= read_bench();
 


  // deallocate memory
  free(mempool);

  return 0;
}
