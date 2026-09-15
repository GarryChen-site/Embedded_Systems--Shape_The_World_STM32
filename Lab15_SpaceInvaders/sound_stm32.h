// sound_stm32.h — Lab 15 Space Invaders, STM32 F446RE port
//
// Replaces the 110 KB Sound.c / Sound.h (TM4C PCM playback).
// Provides the same function names used by SpaceInvaders.c.
// Implementation: short busy-wait sine-wave tones via the R-2R DAC (PB3-PB6).
//
// If no buzzer is connected to the DAC output, the game still runs silently.
#ifndef SOUND_STM32_H
#define SOUND_STM32_H
#include <stdint.h>

// initializes DAC
void Sound_Init(void);
// stub
void Sound_Play(const uint8_t *pt, uint32_t count);
// short high-pitched beep
void Sound_Shoot(void);
// short descending tone
void Sound_Killed(void);
// longer low rumble
void Sound_Explosion(void);
void Sound_Fastinvader1(void);
void Sound_Fastinvader2(void);
void Sound_Fastinvader3(void);
void Sound_Fastinvader4(void);
// UFO appearance
void Sound_Highpitch(void);

#endif