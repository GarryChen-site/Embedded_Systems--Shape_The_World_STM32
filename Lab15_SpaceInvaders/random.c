// random.c — Lab 15 STM32 port
//
// Replaces the TM4C assembly LFSR (random.s).
// Uses XorShift32 — period 2^32-1, same statistical quality.
// API is identical: Random_Init(seed) + Random32().
#include "random.h"

static uint32_t xstate = 1U;

void Random_Init(uint32_t seed) { xstate = seed ? seed : 1U; }

uint32_t Random32(void) {
  // Marsaglia XorShift32
  xstate ^= xstate << 13;
  xstate ^= xstate >> 17;
  xstate ^= xstate << 5;
  return xstate;
}