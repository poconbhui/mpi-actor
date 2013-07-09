#ifndef _HELPER_FUNCTIONS_H
#define _HELPER_FUNCTIONS_H

void initialiseRNG(long *);

void frogHop(float, float, float *, float *, long *);

int willGiveBirth(float, long *);

int willCatchDisease(float, long *);

int willDie(long *);

int getCellFromPosition(float, float);

#endif
