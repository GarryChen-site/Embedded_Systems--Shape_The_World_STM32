// sound_stm32.c — Lab 15 Space Invaders, STM32 F446RE port
//
// Original Sound.c (TM4C): 110 KB of PCM audio tables played via SysTick DAC
// This port: brief sine-wave tones via busy-wait, same function names.
//
// HOW IT WORKS:
//   Each sound function calls tone_burst(half_period, cycles):
//   - Steps through the 32-sample sine table once per cycle
//   - Uses a spin-loop delay between samples to set the frequency
//   - Writes each sample to the R-2R DAC (PB0-PB3) via DAC_Out()
//
// FREQUENCY APPROXIMATION (16 MHz CPU, compiler -O1):
//   Each spin-loop iteration ≈ 4-6 cycles.
//   sample_time ≈ half_period × 5 / 16,000,000 seconds
//   note_freq  ≈ 1 / (32 × sample_time)
//
//   half_period=50  → ~2000 Hz  (high-pitched shoot)
//   half_period=100 → ~1000 Hz  (killed)
//   half_period=200 → ~500 Hz   (invader march)
//   half_period=350 → ~285 Hz   (explosion rumble)
//
// BLOCKING: Sound functions block for their duration (~1-10 ms each).
// This is acceptable because each sound is short and the game loop
// already includes Delay1ms() calls.
//
// References: dac.c (PB3-PB6 R-2R ladder, Lab13 pattern)
#include "sound_stm32.h"
#include "dac.h"
#include <stdint.h>

// 32-point sine wave, 4-bit amplitude (0=min, 15=max)
// Same table as Lab13 DAC / Lab12 TuningFork
static const uint8_t sin_wave32[32] = {
    8, 9, 11, 12, 13, 14, 14, 15, 15, 15, 14, 14, 13, 12, 11, 9,
    8, 7, 5,  4,  3,  2,  2,  1,  1,  1,  2,  2,  3,  4,  5,  7};

// -------------------------------------------------------------------------
// tone_burst — play N complete sine wave cycles at a given pitch
// half_period: spin-loop count per sample (controls frequency)
// cycles:      number of complete 32-sample sine cycles
// -------------------------------------------------------------------------
static void tone_burst(uint32_t half_period, uint32_t cycles) {
  uint32_t total_sample = cycles * 32U;
  for (uint32_t s = 0U; s < total_sample; s++) {
    DAC_Out(sin_wave32[s & 0x1FU]);
    volatile uint32_t t = half_period;
    while (t--)
      ;
  }
  DAC_Out(8U);
}

void Sound_Init(void) {
  // set up PB0-PB3 as outputs
  DAC_Init();
}

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