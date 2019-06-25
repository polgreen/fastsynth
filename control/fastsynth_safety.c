#include "benchmark.h"
__plant_typet nondet_plant();

__controller_typet EXPRESSIONk0(void);
__controller_typet EXPRESSIONk1(void);
__controller_typet EXPRESSIONk2(void);
__controller_typet EXPRESSIONk3(void);
__controller_typet EXPRESSIONk4(void);
__controller_typet EXPRESSIONk5(void);
__controller_typet EXPRESSIONk6(void);
__controller_typet EXPRESSIONk7(void);
__controller_typet EXPRESSIONk8(void);


#define NUMBERLOOPS 5
#define INITIALSTATE_UPPERBOUND (__plant_typet) 0.5
#define INITIALSTATE_LOWERBOUND (__plant_typet)-0.5
#define SAFE_STATE_UPPERBOUND (__plant_typet)10
#define SAFE_STATE_LOWERBOUND (__plant_typet)-10

//other plant variables

 //nondet controller
__controller_typet K_fxp[NSTATES];
__plant_typet _controller_states[NSTATES]; //nondet initial states
__plant_typet _controller_inputs;

//matrices for stability calculation
__plant_typet _AminusBK[NSTATES][NSTATES];
__plant_typet __CPROVER_EIGEN_poly[NSTATES+1u];


//stablity calc
__plant_typet internal_pow(__plant_typet a, unsigned int b){

   __plant_typet acc = 1;
   for (unsigned int i=0; i < b; i++){
    acc = acc*a;
   }
   return acc;
}


int check_stability(){

#define __a __CPROVER_EIGEN_poly
#define __n NSTATES + 1u
   int lines = 2 * __n - 1;
   int columns = __n;
   __plant_typet m[lines][__n];
   int i,j;

   /* to put current values in stability counter-example
    * look for current_stability (use: --no-slice) */
   __plant_typet current_stability[__n];
   for (i=0; i < __n; i++){
     current_stability[i] = __a[i];
   }

   /* check the first constraint condition F(1) > 0 */
   __plant_typet sum = 0;
   for (i=0; i < __n; i++) {
     sum = sum + __a[i];
   }
   if (sum <=0){
     return 0;
   }

   /* check the second constraint condition F(-1)*(-1)^n > 0 */
   sum = 0;
   for (i=0; i < __n; i++){
     sum +=  __a[i]*internal_pow(-1, NSTATES-i);
   }
   sum *= internal_pow(-1, NSTATES);


   if (sum <=0){
    return 0;
   }

   /* check the third constraint condition abs(a0 < an*(z^n)  */
   if( (__a[__n-1] < 0 ? -__a[__n-1] : __a[__n-1]) > __a[0]){
   
     return 0;
   }

   /* check the fourth constraint of condition (Jury Table) */
   for (i=0; i < lines; i++){
    for (j=0; j < columns; j++){
      m[i][j] = 0;
    }
   }
   for (i=0; i < lines; i++){
    for (j=0; j < columns; j++){
     if (i == 0){
      m[i][j] = __a[j];
      continue;
     }
     if (i % 2 != 0 ){
       int x;
       for(x=0; x<columns;x++){
        m[i][x] = m[i-1][columns-x-1];
       }
       columns = columns - 1;
      j = columns;
     }else{
      m[i][j] = m[i-2][j] - (m[i-2][columns]/m[i-2][0]) * m[i-1][j];
     }
    }
   }
   int first_is_positive =  m[0][0] >= 0 ? 1 : 0;
   for (i=0; i < lines; i++){
    if (i % 2 == 0){
	 int line_is_positive = m[i][0] >= 0 ? 1 : 0;
     if (first_is_positive != line_is_positive){
      return 0;
     }
     continue;
    }
   }
   return 1;
}

#define __m _AminusBK


#include "charpolys.h"

void __CPROVER_EIGEN_charpoly(void){

 #if NSTATES==1
  //do nothing
 #else
 generate_charpoly();
#endif

  // Normalise
  __plant_typet max_coefficient=0;
  for (int i = 0; i <= NSTATES; ++i)
    if (max_coefficient < __CPROVER_EIGEN_poly[i])
      max_coefficient=__CPROVER_EIGEN_poly[i];

  for (int i = 0; i <= NSTATES; ++i)
    __CPROVER_EIGEN_poly[i]=__CPROVER_EIGEN_poly[i]/ max_coefficient;
}


void closed_loop(void)
{
    for (int i=0;i<NSTATES; i++) { //rows of B
      for (int j=0; j<NSTATES; j++) { //columns of K
        _AminusBK[i][j] =  _controller_A[i][j] -  _controller_B[i] * (__plant_typet)K_fxp[j];
          }
      }
}


void inputs_equal_ref_minus_k_times_states(void)
  {
    //single input
    __controller_typet result_fxp=0;

     for(int k=0; k<NSTATES; k++)
     {
    	result_fxp = result_fxp +  K_fxp[k] * (__controller_typet)_controller_states[k];
        _controller_inputs = 0 - (__plant_typet)result_fxp;
        __CPROVER_assume(_controller_inputs <= INPUT_UPPERBOUND && _controller_inputs >= INPUT_LOWERBOUND);
  }
 }



void states_equals_A_states_plus_B_inputs(void)
{
    __plant_typet states_equals_A_states_plus_B_inputs_result[NSTATES];
    __CPROVER_array_set(states_equals_A_states_plus_B_inputs_result, 0);

   for(int i=0; i<NSTATES; i++)
   {
    for(int k=0; k<NSTATES; k++) {
      states_equals_A_states_plus_B_inputs_result[i] =  states_equals_A_states_plus_B_inputs_result[i] + (_controller_A[i][k] * _controller_states[k]);
    }
    states_equals_A_states_plus_B_inputs_result[i] = states_equals_A_states_plus_B_inputs_result[i] +  (_controller_B[i]*_controller_inputs);
   }


  for(int i=0; i<NSTATES; i++)
  {
       _controller_states[i] = states_equals_A_states_plus_B_inputs_result[i];
  }
 }



int check_safety(void)
{

  for(int k=0; k<NUMBERLOOPS; k++)
  {

    inputs_equal_ref_minus_k_times_states(); //update inputs one time step //this is still needed for INTERVALS because it enforces bounds on the input
    states_equals_A_states_plus_B_inputs(); //update states one time step

    for(int i=0; i<NSTATES; i++)
    {
      if(_controller_states[i]>SAFE_STATE_UPPERBOUND || _controller_states[i]<SAFE_STATE_LOWERBOUND)
        {return 0;}
    }
  }
  return 1;
}




int main() {

 
  K_fxp[0] =EXPRESSIONk0();
  K_fxp[1] = EXPRESSIONk1();
  K_fxp[2] = EXPRESSIONk2();
  #if NSTATES >3
  K_fxp[3] =EXPRESSIONk3();
  #if NSTATES >4
  K_fxp[1] = EXPRESSIONk4();
  #if NSTATES >5
  K_fxp[2] = EXPRESSIONk5();
  #if NSTATES >6
  K_fxp[0] =EXPRESSIONk6();
  #if NSTATES >7
  K_fxp[1] = EXPRESSIONk7();
  #if NSTATES >8
  K_fxp[2] = EXPRESSIONk8();
  #endif
  #endif
  #endif
  #endif
  #endif
  #endif
  
  for(int i=0; i<NSTATES; i++)
  {
    _controller_states[i]=nondet_plant();
    __CPROVER_assume(_controller_states[i]==INITIALSTATE_UPPERBOUND ||
     _controller_states[i]==INITIALSTATE_LOWERBOUND);
  }
  closed_loop();
 //   __CPROVER_assert(0,"");
  __CPROVER_EIGEN_charpoly();
 //   __CPROVER_assert(0,"");
  __CPROVER_assert(check_stability()==1,"");
  //  __CPROVER_assert(0,"");
  __CPROVER_assert(check_safety()==1,"");
  //__CPROVER_assert(0,"");

 return 0;
}




