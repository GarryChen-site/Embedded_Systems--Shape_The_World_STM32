// dac_hal.h — Lab 15 Space Invaders, HAL version
//
// 4-bit R-2R DAC driver using HAL GPIO (PB0–PB3).
//
// WIRING (R-2R ladder, R = 10 kΩ):
//   PB0  (CN8  pin 5  / Arduino A3) ← bit 0 (LSB)
//   PB1  (CN10 pin 24)              ← bit 1
//   PB2  (CN10 pin 22)              ← bit 2
//   PB4  (CN10 pin 27 / Arduino D12)← bit 3 (MSB)   [PB3 is SWO, reserved]
//   R-2R output → Buzzer+ → Buzzer– → GND
//
// IOC: configure PB0, PB1, PB2, PB4 as GPIO_Output (push-pull, no pull).

#ifndef DAC_HAL_H
#define DAC_HAL_H
#include <stdint.h>

void DAC_HAL_Init(void);            // set all DAC outputs to 0 (silent)
void DAC_HAL_Out(uint8_t data);     // write 4-bit value (0–15) to PB[3:0]

#endif /* DAC_HAL_H */
