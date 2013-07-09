#include <math.h>

#include "ran2.h"
#include "frog-functions.h"

void initialiseRNG(long *seed) {

    /* 
     * Call this **once** at the start of your program **on each process**
     * The input value should be **negative**, **non-zero**, and
     *  **different on every process**
     * You could seed with the value -1-rank where rank
     *  is your MPI rank
     * 
     * ran2 is a flawed RNG and must be used with particular care in parallel
     * however it is perfectly sufficient for this coursework
     * 
     *  printf("Initialising with %d",*idum);
     */
    ran2(seed);
}

void frogHop(float x, float y, float* x_new, float* y_new, long * state){

    /* 
     * You can also use frogHop (0,0,x_start,y_start,idum) to get a random
     * starting position for a frog.
     */

  
    float diff;

    diff=ran2(state); /* don't worry that diff is always >0 this is
                      * dealt with by the periodic BCs */
  
    *x_new=(x+diff)-(int)(x+diff);

    diff=ran2(state);

    *y_new=(y+diff)-(int)(y+diff);

  
}

int willGiveBirth(float avg_pop, long * state){
  
    float tmp;

    tmp=avg_pop/2000.0;

    return(ran2(state)<(atan(tmp*tmp)/(4*tmp)));

}

int willCatchDisease(float avg_inf_level, long * state){

    return(ran2(state)<(atan(((avg_inf_level < 40000 ? avg_inf_level : 40000))/2000.0)/M_PI));

}

int willDie(long * state){

    return(ran2(state)<(0.166666666));

}

int getCellFromPosition(float x, float y){
  
    return((int)(x*4)+4*(int)(y*4));
  
}

