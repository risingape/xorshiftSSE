/*
 * Marsaglia's XORSHIFT RNG implemented using Intel's streaming SIMD extensions (SSE).
 */

#include "xorshiftSSE.h"



/*
 * auxiliary functions to seed the RNG
 */
uint32_t getSeed(uint32_t *seed) {
   int32_t fn;
   uint32_t r;

   fn = open("/dev/urandom", O_RDONLY);
   if(fn == -1) {
       return(1);
   }

   if(read(fn, &r, 4) != 4) {
       return(1);
   }

   close(fn);

   *seed = r;

   return(0);
}


/*
 * The platform independent part of seeding the RNG.
 * Simply filling up the seed array.
 */
uint32_t seed(uint32_t *seeds, uint32_t l) {
        uint32_t _i;
        uint32_t _err = 0;

        for (_i = 0; _i < l; _i++) {
                _err = getSeed(&seeds[_i]);
                if(_err) break;
        }

        return(_err);
}


/*
 * Sequential 32-bit XORSHIFT RNG generating a single random number each call.
 * This RNG uses 5 seeds, change the number of seeds changes the structure of the RNG.
 *
 * This is the "gold" code we are comparing the vectorised implementation to.
 */
uint32_t xorshift32(uint32_t *seeds, uint32_t *sample) {
	uint32_t t;

	t = (seeds[0] ^ (seeds[0] >> 7));
	seeds[0] = seeds[1];
	seeds[1] = seeds[2];
	seeds[2] = seeds[3];
	seeds[3] = seeds[4];
	seeds[4] = (seeds[4] ^ (seeds[4] << 6)) ^ (t ^ (t << 13));
	*sample = (seeds[1] + seeds[1] + 1) * seeds[4];

        return(0);
}


/* 
 * General note on the vectorisation:
 * The SSE registers are 128-bit or 4 32-bit integers in length. This means we 
 * are generating 4 streams of random numbers in parallel. Each stream is
 * seeded by using /dev/urandom.
 */ 

/*
 * First SSE implementation - 1 random number per stream.
 * 
 * Replaced the pumping of the internal state by a ring-buffer to avoid 
 * expensive copying between registers. It requires 4x5 + 1 = 21 seeds where
 * the last seed seeds[20] is the modulo 5 counter.
 */
uint32_t xorshiftsse32(uint32_t *seeds, uint32_t *sample) {
    uint32_t _4 = 0;
    uint32_t _1 = 0;
    __m128i t;
    __m128i s1;
    __m128i s4;
    
    /*
     * Move our seeds array into cache - it is spread out over two cache lines.
     * Since our memory layout of the seeds and samples arrays is linear this
     * should have no effect.
     */
    //_mm_prefetch( &seeds[64], _MM_HINT_T0 );
    //_mm_prefetch( seeds, _MM_HINT_T0 );
    
    _4 = seeds[20];
    
    // update the ring buffer pointer
    seeds[20] += 4;
    _1 = seeds[20];
    
    if(seeds[20] >= 12) {_1 -= 12;} else {_1 += 8;}
    if(seeds[20] >= 20) {seeds[20] -= 20;}
     
    // load the vectors
    t = _mm_load_si128((__m128i*)&seeds[seeds[20]]); 
    s4 = _mm_load_si128((__m128i*)&seeds[_4]);
    s1 = _mm_load_si128((__m128i*)&seeds[_1]);
    
    // sequential code: t = (seeds[0] ^ (seeds[0] >> 7));
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
            
    // this is done by the ring buffer
    //seeds[0] = seeds[1]; seeds[1] = seeds[2]; 
    //seeds[2] = seeds[3]; seeds[3] = seeds[4];

    // sequential code: seeds[4] = (seeds[4] ^ (seeds[4] << 6)) ^ (t ^ (t << 13));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[seeds[20]], s4);
    
    //s4 = _mm_xor_si128( _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) ), _mm_xor_si128(t, _mm_slli_epi32(t, 13) ) );
    
    /*
     * requires SSE 4.1:
     * packed signed multiplication, four packed sets of 32-bit 
     * integers multiplied to give 4 packed 32-bit results.
     *
     * sequential code: *sample = (seeds[1] + seeds[1] + 1) * seeds[4];
     * Fortunately, seed[4] is still in s4, so we don't have to load it again.
     */
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    
    // store s1 in *sample
    _mm_store_si128((__m128i*)sample, s1);
    
    return(0);
}


/*
 * Second implementation - unroll the state pumping logic.
 * This implementation generates 5 random numbers for each of its 4 stream.
 * Basically, we unrolled the ringbuffer logic and no address calculations 
 * are necessary.
 */
uint32_t xorshiftsse32_unrolled(uint32_t *seeds, uint32_t *samples) {

    __m128i t;
    __m128i s1;
    __m128i s4;
    
    /*
     * Move our seeds array into cache - it is spread out over two cache lines.
     * Since our memory layout of the seeds and samples arrays is linear this
     * should have no effect.
     */
    _mm_prefetch( &seeds[16], _MM_HINT_T0 );
    _mm_prefetch( seeds, _MM_HINT_T0 );
    _mm_prefetch( samples, _MM_HINT_T0 );
    
    // round 1
    t = _mm_load_si128((__m128i*)&seeds[0]); 
    s4 = _mm_load_si128((__m128i*)&seeds[16]);
    s1 = _mm_load_si128((__m128i*)&seeds[8]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[0], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[0], s1);
    
    // round 2
    t = _mm_load_si128((__m128i*)&seeds[4]); 
    s4 = _mm_load_si128((__m128i*)&seeds[0]);
    s1 = _mm_load_si128((__m128i*)&seeds[12]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[4], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[4], s1);
    
    // round 3
    t = _mm_load_si128((__m128i*)&seeds[8]); 
    s4 = _mm_load_si128((__m128i*)&seeds[4]);
    s1 = _mm_load_si128((__m128i*)&seeds[16]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[8], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[8], s1);
    
    // round 4
    t = _mm_load_si128((__m128i*)&seeds[12]); 
    s4 = _mm_load_si128((__m128i*)&seeds[8]);
    s1 = _mm_load_si128((__m128i*)&seeds[0]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[12], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[12], s1);
    
    // round 5
    t = _mm_load_si128((__m128i*)&seeds[16]); 
    s4 = _mm_load_si128((__m128i*)&seeds[12]);
    s1 = _mm_load_si128((__m128i*)&seeds[4]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[16], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[16], s1);
    
    return(0);
}


/*
 * Thrid implementation - function pointers.
 * While the second implemenatation is very efficient, it generates many random
 * numbers at once. This implementation tries to eliminate this.
 */


/*
 * This function pointer is the entry point for the user.
 * It is changed at the end of each call to the RNG.
 * We initialise it with the address of the first xorshift function.
 */
uint32_t (*pt)(uint32_t*, uint32_t*) = xorshiftR0; 

 
uint32_t xorshiftR0(uint32_t *seeds, uint32_t *samples) {
    __m128i t;
    __m128i s1;
    __m128i s4;
    
    // round 1
    t = _mm_load_si128((__m128i*)&seeds[0]); 
    s4 = _mm_load_si128((__m128i*)&seeds[16]);
    s1 = _mm_load_si128((__m128i*)&seeds[8]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[0], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)samples, s1);

    // switch the function pointer
    pt = xorshiftR1;    
    
    return(0);
}



uint32_t xorshiftR1(uint32_t *seeds, uint32_t *samples) {
    __m128i t;
    __m128i s1;
    __m128i s4;
    
    t = _mm_load_si128((__m128i*)&seeds[4]); 
    s4 = _mm_load_si128((__m128i*)&seeds[0]);
    s1 = _mm_load_si128((__m128i*)&seeds[12]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[4], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[4], s1);
    
    // switch the function pointer
    pt = xorshiftR2;       
    
    return(0);
}
 

uint32_t xorshiftR2(uint32_t *seeds, uint32_t *samples) {
    __m128i t;
    __m128i s1;
    __m128i s4;
    
    t = _mm_load_si128((__m128i*)&seeds[8]); 
    s4 = _mm_load_si128((__m128i*)&seeds[4]);
    s1 = _mm_load_si128((__m128i*)&seeds[16]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[8], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[8], s1);
    
    // switch the function pointer
    pt = xorshiftR3;     
    
    return(0);
}
 
 
uint32_t xorshiftR3(uint32_t *seeds, uint32_t *samples) {
    __m128i t;
    __m128i s1;
    __m128i s4;
    
    t = _mm_load_si128((__m128i*)&seeds[12]); 
    s4 = _mm_load_si128((__m128i*)&seeds[8]);
    s1 = _mm_load_si128((__m128i*)&seeds[0]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[12], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[12], s1);
    
    // switch the function pointer
    pt = xorshiftR4;     
    
    return(0);
}
 

uint32_t xorshiftR4(uint32_t *seeds, uint32_t *samples) {
    __m128i t;
    __m128i s1;
    __m128i s4;
    
    t = _mm_load_si128((__m128i*)&seeds[16]); 
    s4 = _mm_load_si128((__m128i*)&seeds[12]);
    s1 = _mm_load_si128((__m128i*)&seeds[4]); // index after pumping
    t = _mm_xor_si128(t, _mm_srli_epi32(t, 7));
    s4 = _mm_xor_si128(s4, _mm_slli_epi32(s4, 6) );
    t = _mm_xor_si128(t, _mm_slli_epi32(t, 13) );
    s4 = _mm_xor_si128(s4, t);
    _mm_store_si128((__m128i*)&seeds[16], s4);
    s1 = _mm_mullo_epi32(_mm_add_epi32(_mm_add_epi32(s1, s1), _mm_set1_epi32(1)), s4);
    _mm_store_si128((__m128i*)&samples[16], s1);
    
    // switch the function pointer
    pt = xorshiftR0;    
    
    return(0);
}


