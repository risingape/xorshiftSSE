
== Vectorised implementation of Marsaglia's XORSHIFT random number generator ==

This vectorised version of Marsaglia's XORSHIFT random number generator (RNG) is implemented using Intel's streaming SIMD extensions (SSE) version 4.1. The RNG generates 32-bit random samples. The width of the vector registers is 128 bit giving a vector length of 4 32-bit integers.


== Very brief overview of Intels streaming SIMD extensions (SSE) ==


== SSE based implementations == 
For a description of Marsaglia's XORSHIFT RNG, please refer to <> or the documentation of the TinyRNG library <link goe here>.

= Ring buffer implementation
First SSE implementation



= Unroll the ring buffer
Second SSE implementation
Decrese the granularity

= Replace the modulo counter by function pointers
Make it more fine grained again

== Compiling ==
gcc test.c xorshiftSSE.c -O3 -msse4.1 -I.


== Timing results ==

guido@linux-9cge:~/scratch/xorshiftSSE/remake> ./a.out 
xorshift32 1000000000 samples in 4.162631 sec.
xorshift32SSE 1000000000 samples in 1.987162 sec. speedup 2.095
xorshift32SSE unrolled 1000000000 samples in 1.094806 sec. speedup 3.802
xorshift function pointers 1000000000 samples in 1.286171 sec.speedup 3.236
guido@linux-9cge:~/scratch/xorshiftSSE/remake>
xorshift32 1000000000 samples in 3.843894 sec.
xorshift32SSE 1000000000 samples in 1.756420 sec. speedup 2.188
xorshift32SSE unrolled 1000000000 samples in 0.918752 sec. speedup 4.184
xorshift function pointers 1000000000 samples in 1.282542 sec. speedup 2.997
guido@linux-9cge:~/scratch/xorshiftSSE/remake> ./a.out 
xorshift32 1000000000 samples in 3.957081 sec.
xorshift32SSE 1000000000 samples in 1.987700 sec. speedup 1.991
xorshift32SSE unrolled 1000000000 samples in 1.095842 sec. speedup 3.611
xorshift function pointers 1000000000 samples in 1.285295 sec. speedup 3.079

Average speedup:
xorshift32SSE 2.091
xorshift32SSE unrolled 3.866
xorshift function pointers 3.104

// old data
guido@linux-9cge:~/scratch/xorshiftSSE/remake> ./a.out 
xorshift32 10000000000 samples in 28.207458 sec.
xorshift32SSE 10000000000 samples in 14.798805 sec.
xorshift32SSE_unroll 10000000000 samples in 10.772815 sec.
xorshift function pointers 10000000000 samples in 12.573911 sec.


guido@linux-9cge:~/scratch/xorshiftSSE/remake> ./a.out 
xorshift32 10000000000 samples in 28.177029 sec.
xorshift32SSE 10000000000 samples in 14.541718 sec.
xorshift32SSE_unroll 10000000000 samples in 10.474033 sec.
xorshift function pointers 10000000000 samples in 12.741058 sec.

