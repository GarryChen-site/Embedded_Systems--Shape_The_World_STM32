// sound_hal.c — Lab 15 Space Invaders, HAL version
//
// Replaces sound_stm32.c.  Uses dac_hal.c (PB0–PB3 R-2R) and a spin-loop
// for frequency control — identical technique to the bare-metal version.
//
// TIMING (16 MHz CPU, -O1):
//   Each spin-loop iteration ≈ 4–6 cycles.
//   sample_time ≈ half_period × 5 / 16 000 000 s
//   note_freq   ≈ 1 / (32 × sample_time)
//
//   half_period= 50 → ~2000 Hz (shoot)
//   half_period=130 → ~ 800 Hz (killed)
//   half_period=350 → ~ 285 Hz (explosion)
//
// NOTE: HAL SysTick ISR fires every 1 ms (≈ 16 000 cycles overhead per ms).
// For a 1 kHz tone (half_period≈100), the ISR overhead adds ~1.6% frequency
// error — imperceptible for game sound effects.

#include "sound_hal.h"
#include "dac_hal.h"
#include <stdint.h>

static const uint8_t sin_wave32[32] = {
    8, 9, 11, 12, 13, 14, 14, 15, 15, 15, 14, 14, 13, 12, 11, 9,
    8, 7, 5,  4,  3,  2,  2,  1,  1,  1,  2,  2,  3,  4,  5,  7};

// -------------------------------------------------------------------------
// tone_burst — play N full sine cycles at given pitch
// half_period : spin-loop count per sample (controls frequency)
// cycles      : number of complete 32-sample sine wave cycles
// -------------------------------------------------------------------------
static void tone_burst(uint32_t half_period, uint32_t cycles) {
  uint32_t total = cycles * 32U;
  for (uint32_t s = 0U; s < total; s++) {
    DAC_HAL_Out(sin_wave32[s & 0x1FU]);
    volatile uint32_t t = half_period;
    while (t--)
      ;
  }
  // return to DC mid-point (silence)
  DAC_HAL_Out(8U);
}

// -------------------------------------------------------------------------
void Sound_HAL_Init(void) {
  DAC_HAL_Init(); // zero outputs (silent)
}

// PCM stub — not implemented on STM32
void Sound_Play(const uint8_t *pt, uint32_t count) {
  (void)pt;
  (void)count;
}

void Sound_Shoot(void) { tone_burst(50U, 4U); }
void Sound_Killed(void) { tone_burst(130U, 5U); }
void Sound_Explosion(void) { tone_burst(350U, 6U); }
void Sound_Fastinvader1(void) { tone_burst(200U, 2U); }
void Sound_Fastinvader2(void) { tone_burst(170U, 2U); }
void Sound_Fastinvader3(void) { tone_burst(140U, 2U); }
void Sound_Fastinvader4(void) { tone_burst(110U, 2U); }
void Sound_Highpitch(void) { tone_burst(30U, 8U); }