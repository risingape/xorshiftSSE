#ifndef XORSHIFTSSE_H
#define XORSHIFTSSE_H

#include <stdint.h>
#include <fcntl.h>

// SSE intrinsics
#include <xmmintrin.h>
#include <smmintrin.h>

uint32_t getSeed(uint32_t *seed);
uint32_t seed(uint32_t *seeds, uint32_t l);

// sequential
uint32_t xorshift32(uint32_t *seeds, uint32_t *sample);
uint32_t xorshiftsse32(uint32_t *seeds, uint32_t *sample);
uint32_t xorshiftsse32_unrolled(uint32_t *seeds, uint32_t *samples);
 

// function pointer based implementation 
uint32_t xorshiftR0(uint32_t *seeds, uint32_t *samples);
uint32_t xorshiftR1(uint32_t *seeds, uint32_t *samples);
uint32_t xorshiftR2(uint32_t *seeds, uint32_t *samples);
uint32_t xorshiftR3(uint32_t *seeds, uint32_t *samples);
uint32_t xorshiftR4(uint32_t *seeds, uint32_t *samples);

// This function pointer is the entry point for the user.
// It is changed at the end of each call to the RNG.
extern uint32_t (*pt)(uint32_t*, uint32_t*); 

#endif
