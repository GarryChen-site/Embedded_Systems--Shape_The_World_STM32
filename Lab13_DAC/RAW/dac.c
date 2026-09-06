// dac.c — Lab 13 DAC, STM32 F446RE port
//
// Original: 4-bit DAC on PB0–PB3 (TM4C Port B)
// Ported to: 4-bit DAC on PB3–PB6 (STM32 GPIOB bits [6:3])
//
// R-2R RESISTOR LADDER (4-bit, using 10kΩ only):
// 2R = two 10kΩ in series = 20kΩ equivalent
// R  = one  10kΩ
//
//   PB3(bit0,LSB)─[10k]─[10k]─┐
//                               ├──[10k]──┐
//   PB4(bit1)    ─[10k]─[10k]─┘          │
//                        [10k]            ├──[10k]──┐
//                        [10k]            │          │
//   PB5(bit2)    ─[10k]─[10k]─┐  │      │          ├──[10k]──┐
//                               ├──[10k]──┘          │          │
//   PB6(bit3,MSB)─[10k]─[10k]─┘  [10k]              │          ├── VOUT
//                                  [10k]             GND        │
//                                   │           (termination)  [10k]
//                                  GND                         [10k]
//                             (termination)                      │
//                                                               GND
//
//   VOUT ──[220Ω]── Buzzer+ ── Buzzer− ── GND
//          (limits current; smooths staircase waveform)
//
// Parts needed:
//    8× 10kΩ  — four 2R shunt paths (2 resistors each bit: PB3/4/5/6)
//    2× 10kΩ  — termination 2R at N0 (LSB node to GND)
//    3× 10kΩ  — R rail connections (N0→N1, N1→N2, N2→VOUT)
//    ───────
//   13× 10kΩ  total
//    1× 220Ω  — output series resistor to buzzer
// Output range: 0 V to 3.3 × 15/16 = 3.09 V
// Resolution:   3.3 / 16 = 0.206 V per step
// Source impedance: ~R = 10 kΩ (Thevenin from R-2R)
//
// Pin locations
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
#define DAC_MASK (0xFU << 3) // bits 3..6 (PB3..PB6)

void DAC_init(void) {
  RCC_AHB1ENR |= (1U << 1);

  volatile uint32_t dummy = RCC_AHB1ENR;

  (void)dummy;

  GPIOB_MODER &= ~(0xFFU << 6); // clear MODER[13:6] (PB3..PB6)
  GPIOB_MODER |= (0x55U << 6);  // set output mode

  GPIOB_ODR &= ~DAC_MASK;
}

// DAC_Out: write 4-bit value (0–15) to PB[6:3]
// Bit 0 of data → PB3 (LSB), Bit 3 → PB6 (MSB)
void DAC_Out(uint8_t data) {
  GPIOB_ODR = (GPIOB_ODR & ~DAC_MASK) | ((data & 0xFU) << 3);
}
