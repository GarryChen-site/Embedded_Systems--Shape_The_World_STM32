// dac.c — Lab 13 DAC, STM32 F446RE port
//
// Original: 4-bit DAC on PB0–PB3 (TM4C Port B)
// Ported to: 4-bit DAC on PB3–PB6 (STM32 GPIOB bits [6:3])
//
// R-2R RESISTOR LADDER (4-bit, R=10kΩ, 2R=20kΩ):
//
//   PB3(bit0,LSB)─[20k]─┐
//                        ├─[10k]─┐
//   PB4(bit1)    ─[20k]─┘        │
//                        [20k]   ├─[10k]─┐
//                         │      │        │
//   PB5(bit2)    ─[20k]─┐ GND   │        ├─[10k]─┐
//                        ├─[10k]─┘        │        │
//   PB6(bit3,MSB)─[20k]─┘        [20k]   │        ├─ VOUT ── Buzzer+ ── Buzzer-
//   ── GND
//                                  │      GND      │
//                                  GND           [20k]
//                                                  │
//                                                 GND
//
// Output range: 0 V to 3.3 × 15/16 = 3.09 V
// Resolution:   3.3 / 16 = 0.206 V per step
// Source impedance: ~R = 10 kΩ (Thevenin from R-2R)
//
// Pin locations :
//   PB3  CN9 pin 3  (D3)
//   PB4  CN9 pin 5  (D5)
//   PB5  CN9 pin 4  (D4)
//   PB6  CN9 pin 9  (D10)
//

#include "dac.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOB_MODER (*((volatile uint32_t *)0x40020400U))
#define GPIOB_ODR (*((volatile uint32_t *)0x40020414U))

// DAC uses GPIOB bits [6:3] = PB3(bit0) PB4(bit1) PB5(bit2) PB6(bit3)
#define DAC_MASK (0xFU << 3) // bits 6,5,4,3

void DAC_Init(void) {
  RCC_AHB1ENR |= (1U << 1);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // PB3,PB4,PB5,PB6 → output (01)
  GPIOB_MODER &= ~(0xFFU << 6); // clear MODER[13:6] (PB3..PB6)
  GPIOB_MODER |= (0x55U << 6);  // 01 01 01 01 = output for each

  GPIOB_ODR &= ~DAC_MASK; // initialize DAC to 0 (silent)
}

// DAC_Out: write 4-bit value (0–15) to PB[6:3]
// Bit 0 of data → PB3 (LSB), Bit 3 → PB6 (MSB)
void DAC_Out(uint8_t data) {
  GPIOB_ODR = (GPIOB_ODR & ~DAC_MASK) | ((data & 0xFU) << 3);
}