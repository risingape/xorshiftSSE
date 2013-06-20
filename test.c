#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "xorshiftSSE.h"
#include "timing.h"

/* The number of samples to be computed */
#define NSAMPLES 1000000000

int main() {
  double timer = 0.0;
  uint64_t i = 0;
 
  uint32_t *seeds __attribute__((aligned(16)));
  uint32_t *samples __attribute__((aligned(16)));
  struct timespec start, stop, timeDiff;
  
  seeds = (uint32_t*)malloc(21 * sizeof(uint32_t));
  samples = (uint32_t*)malloc(20 * sizeof(uint32_t));
  
  seed(seeds, 20);
  seeds[20] = 0; /* this is the modulo counter of the first SSE implementation */
  
  /*
   * The sequential XORSHIFT algorithm.
   */

  /* start timer */
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  
  for(i = 0; i < NSAMPLES; i ++) {
     xorshift32(seeds, &samples[0]); 
  }
  
  /* stop timer */
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
  timeDiff = diff(start, stop);
  timer = (float) timeDiff.tv_sec + (float) timeDiff.tv_nsec / 1000000000.0f;
  printf("xorshift32 %llu samples in %f sec.\n", NSAMPLES, timer);

  /*
   * First SSE implementation - using a ring buffer and modulo counter
   * We compute 4 random numbers, or 1 per stream, in each call.
   */

  /* start timer */
  timer = 0.0;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  
  for(i = 0; i < NSAMPLES / 4; i ++) {
      xorshiftsse32(seeds, samples);
  }
  
  /* stop timer */
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
  timeDiff = diff(start, stop);
  timer = (float) timeDiff.tv_sec + (float) timeDiff.tv_nsec / 1000000000.0f;
  printf("xorshift32SSE %llu samples in %f sec.\n", NSAMPLES, timer);

  /*
   * Second SSE implementation - unrolled the modulo counter
   *
   * No more address computation, but we generate 5x4=20 random samples
   * in each call.
   */
 
  /* start timer */
  timer = 0.0;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
 
  for(i = 0; i < NSAMPLES / 20; i ++) {
      xorshiftsse32_unrolled(seeds, samples);
  }                                                                                                                                              
 
  /* stop timer */
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
  timeDiff = diff(start, stop);
  timer = (float) timeDiff.tv_sec + (float) timeDiff.tv_nsec / 1000000000.0f;
  printf("xorshift32SSE unrolled %llu samples in %f sec.\n", NSAMPLES, timer);

  /*
   * Third SSE implementation - split the RNG into 5 functions. Each function
   * Computes one "round". Before we return from the "round" function, we switch
   * a function pointer to point to the next "round"'s function.
   * We compute 4 random numbers, or 1 per stream, in each call.
   */

  /* start timer */
  timer = 0.0;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  
  /* Set up the function pointer */
  pt = xorshiftR0;  

  /* This RNG generates 4 random numbers per iteration */
  for(i = 0; i < NSAMPLES / 4; i ++) {
      pt(seeds, samples);
  }
  
  /* stop timer */
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
  timeDiff = diff(start, stop);
  timer = (float) timeDiff.tv_sec + (float) timeDiff.tv_nsec / 1000000000.0f;
  printf("xorshift function pointers %llu samples in %f sec.\n", NSAMPLES, timer);
  
  /* Clean-up */
  free(seeds);
  free(samples);

  return(0);
}

