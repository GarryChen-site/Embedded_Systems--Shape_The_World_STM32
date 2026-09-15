// random.h — Lab 15 STM32 port
// Replaces the TM4C assembly random.s / random.h
// Provides a 32-bit XorShift PRNG with identical API.
#ifndef RANDON_H
#define RANDON_H
#include <stdint.h>

void Random_Init(uint32_t seed);
uint32_t Random32(void);


#endif
