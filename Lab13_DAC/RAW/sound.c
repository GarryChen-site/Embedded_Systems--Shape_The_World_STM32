
#include "sound.h"

#define STK_CTRL (*((volatile uint32_t *)0xE000E010U))
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U))
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))
#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20U))

static const uint8_t sin_wave32[32] = {
    8, 9, 11, 12, 13, 14, 14, 15, 15, 15, 14, 14, 13, 12, 11, 9,
    8, 7, 5,  4,  3,  2,  2,  1,  1,  1,  2,  2,  3,  4,  5,  7};

volatile uint32_t soundOff = 1; // 1 = slient, 0 = playing
volatile uint32_t stateDAC = 0; // current index into sin_wave32 (0-31)
