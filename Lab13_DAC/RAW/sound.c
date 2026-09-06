// sound.c — Lab 13 DAC, STM32 F446RE port
//
// Original: SysTick at 80 MHz, steps through 32-point sine table → DAC
// (PB0-PB3) Ported to: SysTick at 16 MHz, same sine table → DAC (PB3-PB6)
//
// HOW THE SINE WAVE IS PRODUCED:
//   The 32-entry table contains one cycle of a sine wave, quantized to 4 bits
//   (0–15). Each SysTick interrupt advances the table index by 1 and writes the
//   value to the DAC. After 32 steps the index wraps, completing one full sine
//   cycle.
//
//   SysTick rate = 16,000,000 / RELOAD
//   Note freq    = SysTick rate / 32
//   → RELOAD = 16,000,000 / (note_freq × 32)
//
// PERIOD TABLE (16 MHz, 32 samples):
//   C (523.251 Hz): RELOAD=956  → 16M/956/32  = 523.26 Hz
//   D (587.330 Hz): RELOAD=851  → 16M/851/32  = 587.67 Hz
//   E (659.255 Hz): RELOAD=758  → 16M/758/32  = 659.63 Hz
//   G (783.991 Hz): RELOAD=638  → 16M/638/32  = 783.70 Hz
//
#include "sound.h"
#include "dac.h"

#define STK_CTRL (*((volatile uint32_t *)0xE000E010U))
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U))
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))
#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20U))

// =========================================================================
// 32-point sine wave, 4-bit amplitude (0–15)
// =========================================================================
static const uint8_t sin_wave32[32] = {
    8,  9,  11, 12, 13, 14, 14, 15, // Q1: rising  (0° → 90°)
    15, 15, 14, 14, 13, 12, 11, 9,  // Q2: falling (90° → 180°)
    8,  7,  5,  4,  3,  2,  2,  1,  // Q3: falling (180° → 270°)
    1,  1,  2,  2,  3,  4,  5,  7}; // Q4: rising  (270° → 360°)

volatile uint32_t soundOff = 1; // 1 = slient, 0 = playing
volatile uint32_t stateDAC = 0; // current index into sin_wave32 (0-31)

void Sound_Init(void) {

  stateDAC = 0;
  soundOff = 1;
  DAC_Init();

  STK_CTRL = 0;      // disable SysTick during setup
  STK_VAL = 0;       // clear counter
  STK_LOAD = 16000U; // ~100 Hz ISR rate at startup (harmless)
  SCB_SHPR3 = (SCB_SHPR3 & 0x00FFFFFFU) |
              (1U << 29); // SysTick priority 1
                          // original: NVIC_SYS_PRI3 | 0x20000000
  STK_CTRL = 0x00000007U; // ENABLE | TICKINT | CLKSOURCE(AHB)
}

// =========================================================================
// Sound_Tone — change SysTick period to produce a note
// =========================================================================
void Sound_Tone(uint32_t period) {
  if (STK_LOAD == period) {
    return; // already playing this note, do nothing
  }

  STK_CTRL = 0; // disable while reconfiguring
  soundOff = 0;
  stateDAC = 0;
  STK_VAL = 0;
  STK_LOAD = period;
  SCB_SHPR3 &= 0x00FFFFFFU; // raise to priority 0 while playing
  STK_CTRL = 0x00000007U;
}

// =========================================================================
// Sound_Off — silence the DAC
// =========================================================================
void Sound_Off(void) {
  soundOff = 1;
  Sound_Tone(0);
}

// =========================================================================
// SysTick_Handler — called at (16MHz / RELOAD) Hz
// Advances sine table index and writes to DAC
// =========================================================================
void SysTick_Handler(void) {
  if (soundOff) {
    DAC_Out(0); // silence
  } else {
    stateDAC = (stateDAC + 1) & 0x1FU; // advance, wrap at 32
    DAC_Out(sin_wave32[stateDAC]);
  }
}