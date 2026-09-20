// sound_hal.h — Lab 15 Space Invaders, HAL version
//
// Drop-in replacement for sound_stm32.h.
// Uses dac_hal.c (PB0–PB3 R-2R DAC) and a spin-loop for pitch control.
// All function names are identical to the original Sound.h (TM4C) API.
//
// Note: HAL SysTick fires every 1 ms and may introduce small frequency jitter
// in tone_burst(). The effect is negligible for game sound effects.

#ifndef SOUND_HAL_H
#define SOUND_HAL_H
#include <stdint.h>

void Sound_HAL_Init(void);                               // initialise DAC
void Sound_Play(const uint8_t *pt, uint32_t count);     // stub (no PCM)
void Sound_Shoot(void);
void Sound_Killed(void);
void Sound_Explosion(void);
void Sound_Fastinvader1(void);
void Sound_Fastinvader2(void);
void Sound_Fastinvader3(void);
void Sound_Fastinvader4(void);
void Sound_Highpitch(void);

#endif /* SOUND_HAL_H */
