
#include "sound.h"
#include "dac.h"

#define STK_CTRL (*((volatile uint32_t *)0xE000E010U))
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U))
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))
#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20U))

static const uint8_t sin_wave32[32] = {
    8, 9, 11, 12, 13, 14, 14, 15, 15, 15, 14, 14, 13, 12, 11, 9,
    8, 7, 5,  4,  3,  2,  2,  1,  1,  1,  2,  2,  3,  4,  5,  7};

volatile uint32_t soundOff = 1; // 1 = slient, 0 = playing
volatile uint32_t stateDAC = 0; // current index into sin_wave32 (0-31)

void Sound_Init(void) {

  stateDAC = 0;
  soundOff = 1;
  DAC_Init();

  STK_CTRL = 0;
  STK_VAL = 0;
  STK_LOAD = 16000;
  SCB_SHPR3 = (SCB_SHPR3 & 0x00FFFFFFU) | (1U << 29);

  STK_CTRL = 0x00000007U;
}

void Sound_Tone(uint32_t period) {
  if (STK_LOAD == period)
    return; // already playing this note, do nothing

  STK_CTRL = 0; // disable while reconfiguring
  soundOff = 0;
  stateDAC = 0;
  STK_VAL = 0;
  STK_LOAD = period;
  SCB_SHPR3 &= 0x00FFFFFFU; // raise to priority 0 while playing
  STK_CTRL = 0x00000007U;
}

void Sound_Off(void) {
  soundOff = 1;
  Sound_Tone(0);
}

void SysTick_Handler(void) {
  if (soundOff) {
    DAC_Out(0); // silence
  } else {
    stateDAC = (stateDAC + 1) & 0x1FU; // advance, wrap at 32
    DAC_Out(sin_wave32[stateDAC]);
  }
}