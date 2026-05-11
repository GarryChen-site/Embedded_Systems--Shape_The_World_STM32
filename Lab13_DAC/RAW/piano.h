#ifndef PIANO_H
#define PIANO_H
#include <stdint.h>


#define SILENCE  0x00
#define DO       0x01   // C4 @ 523.251 Hz
#define RE       0x02   // D4 @ 587.330 Hz
#define MI       0x04   // E4 @ 659.255 Hz
#define SOL      0x08   // G4 @ 783.991 Hz



void Piano_Init(void);
uint8_t Piano_In(void);



#endif