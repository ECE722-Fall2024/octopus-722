#include <iostream>
#include <cstdlib>
#include <stdbool.h>
#include <stdint.h>
using namespace std;



#define KB 1024
#define MEMORYPOOL_SIZE 256*KB
#define STRIDE 4
#define NUM_ITERATIONS 1


int* mempool = 0;

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

  int64_t sum=0;
  SIM_BEGIN(1);
  for (int i = 0; i < MEMORYPOOL_SIZE/4; i+=STRIDE/4) {
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
    cout << "Memory Allocation Failed";
    exit(1);
  }

  


  for (int i = 0; i < MEMORYPOOL_SIZE/4; i+=STRIDE/4) {
    mempool[i] = i;
  }
  int64_t sum=0;
  
  
  for (int j =0; j<NUM_ITERATIONS;j++)
      sum+= read_bench();
 


  // deallocate memory
  free(mempool);

  return 0;
}
