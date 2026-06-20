
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