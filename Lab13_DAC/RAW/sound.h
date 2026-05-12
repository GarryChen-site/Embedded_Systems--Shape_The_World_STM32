#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

// Period = 16,000,000 / (note_freq × 32_samples)
// SysTick fires 32 times per sine wave cycle → produces note_freq Hz output
//
//  Note  Target Hz   Period    Actual Hz   Error
//  C     523.251     956       523.26      +0.01 Hz
//  D     587.330     851       587.67      +0.34 Hz
//  E     659.255     758       659.63      +0.38 Hz
//  G     783.991     638       783.70      -0.29 Hz
#define DO_PERIOD   956U   // C4
#define RE_PERIOD   851U   // D4
#define MI_PERIOD   758U   // E4
#define SOL_PERIOD  638U   // G4

void Sound_Init(void);
void Sound_Tone(uint32_t period);
void Sound_Off(void);

#endif