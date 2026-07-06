
#include "sound_stm32.h"
#include "dac.h"
#include <stdint.h>

static const uint8_t sin_wave32[32] = {
    8, 9, 11, 12, 13, 14, 14, 15, 15, 15, 14, 14, 13, 12, 11, 9,
    8, 7, 5,  4,  3,  2,  2,  1,  1,  1,  2,  2,  3,  4,  5,  7};

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

void Sound_Init(void) { DAC_Init(); }

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
// UFO
void Sound_Highpitch(void) { tone_burst(30U, 8U); }