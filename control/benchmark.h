#ifndef BENCHMARK_H_ 
#define BENCHMARK_H_ 

// time discretisation 1.000  
#ifndef INT_BITS 
#define INT_BITS 8
#define FRAC_BITS 8
#define _PLANT_TOTAL_BITS 32 
#define _PLANT_RADIX_BITS 16
#endif
typedef __CPROVER_fixedbv[_PLANT_TOTAL_BITS][_PLANT_RADIX_BITS] __plant_typet;  
typedef __CPROVER_fixedbv[INT_BITS+FRAC_BITS][FRAC_BITS] __controller_typet;  
#define NSTATES 2 
#define NINPUTS 1 
#define NOUTPUTS 1
#define INPUT_UPPERBOUND (__plant_typet)10000
#define INPUT_LOWERBOUND (__plant_typet)-10000
const __plant_typet _controller_A[NSTATES][NSTATES] = {{ 9.012249e-01,  1.342930e-08},
{ 7.450581e-09,  0}};
const __plant_typet _controller_B[NSTATES] = {128, 0};
const __plant_typet __char_poly_const[3][3] = 
 {{ 1,  0,  0},
{ -9.012249e-01,  128,  0},
{ -1.000561e-16,  0,  9.536743e-07}};

#endif /*BENCHMARK_H_*/