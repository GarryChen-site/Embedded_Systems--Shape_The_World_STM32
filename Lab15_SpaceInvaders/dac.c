// dac.c — Lab 13 DAC, STM32 F446RE port
//
// Original: 4-bit DAC on PB0–PB3 (TM4C Port B)
// Ported to: 4-bit DAC on PB0–PB3 (STM32 GPIOB bits [3:0])
//
// Pin conflict avoidance:
//   PB6/PB7 = I2C1_SCL/SDA (SSD1306 OLED) → DAC must NOT use PB3–PB6.
//   PB0–PB3 are free of I2C and SWD and are safe for DAC output.
//
// R-2R RESISTOR LADDER (4-bit, R=10kΩ, 2R=20kΩ):
//
//   PB0(bit0,LSB)─[20k]─┬─[10k]─┬─[10k]─┬─[10k]─┬─ VOUT ── Buzzer+
//   PB1(bit1)    ─[20k]─┘        │        │        │
//                            [20k]   │       [20k]     [20k]
//                             │     [20k]    │          │
//   PB2(bit2)    ─[20k]─┬─[10k]─┘  │    GND      GND
//                             │     GND
//   PB3(bit3,MSB)─[20k]─┘        GND
//
// Output range: 0 V to 3.3 × 15/16 = 3.09 V
// Resolution:   3.3 / 16 = 0.206 V per step
//
// Pin locations (Nucleo-F446RE, UM1724):
//   PB0  Arduino A3   (CN8  pin 5)
//   PB1  Morpho       (CN10 pin 24)
//   PB2  Morpho       (CN10 pin 22)
//   PB3  Arduino D3   (CN10 pin 26)  ← MSB
//
// CHANGED from Lab13 PB3–PB6 → PB0–PB3 to free PB6 for I2C1_SCL.
//

#include "dac.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOB_MODER (*((volatile uint32_t *)0x40020400U))
#define GPIOB_ODR (*((volatile uint32_t *)0x40020414U))

// DAC uses GPIOB bits [3:0] = PB0(bit0) PB1(bit1) PB2(bit2) PB3(bit3)
#define DAC_MASK (0xFU << 0) // bits 3,2,1,0

void DAC_Init(void) {
  RCC_AHB1ENR |= (1U << 1); // enable GPIOB clock  (RM0390 §6.3.10)
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // PB0,PB1,PB2,PB3 → output (01)  (RM0390 §8.4.1)
  GPIOB_MODER &= ~(0xFFU << 0); // clear MODER[7:0] (PB0..PB3)
  GPIOB_MODER |= (0x55U << 0);  // 01 01 01 01 = output for each

  GPIOB_ODR &= ~DAC_MASK; // initialize DAC to 0 (silent)
}

// DAC_Out: write 4-bit value (0–15) to PB[3:0]
// Bit 0 of data → PB0 (LSB), Bit 3 → PB3 (MSB)
void DAC_Out(uint8_t data) {
  GPIOB_ODR = (GPIOB_ODR & ~DAC_MASK) | ((data & 0xFU) << 0);
}
