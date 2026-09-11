// dac.c — Lab 13 DAC, STM32 F446RE port
//
// Original: 4-bit DAC on PB0–PB3 (TM4C Port B)
// Ported to: 4-bit DAC on PB0, PB4, PB5, PB6 (STM32 GPIOB)
//
// NOTE: PB3 is reserved as SYS_JTDO-SWO (debug trace) on Nucleo-F446RE.
//       D0 (LSB) is therefore mapped to PB0 (Arduino A3, CN8 pin A3).
//       Pins are non-consecutive: PB0(D0), PB4(D1), PB5(D2), PB6(D3).
//
// R-2R RESISTOR LADDER (4-bit, using 10kΩ only):
// 2R = two 10kΩ in series = 20kΩ equivalent
// R  = one  10kΩ
//
//   PB0(bit0,LSB)─[10k]─[10k]─┬
//                           ├──[10k]──┬
//   PB4(bit1)    ─[10k]─[10k]─┘       │
//                      [10k]       ├──[10k]──┬
//                      [10k]       │           │
//   PB5(bit2)    ─[10k]─[10k]─┬  │          ├──[10k]── VOUT
//                           ├──[10k]──┘          │
//   PB6(bit3,MSB)─[10k]─[10k]─┘  [10k]            [10k]
//                              [10k]             [10k]
//                               │                │
//                              GND             (ladder GND)
//
//   VOUT ───────── Buzzer+ ── Buzzer− ── GND
//   (220Ω removed; 10kΩ source impedance limits current adequately)
//
// Parts: 13× 10kΩ total (same count as before, just re-routed)
//
// Output range: 0 V to 3.3 × 15/16 = 3.09 V
// Resolution:   3.3 / 16 = 0.206 V per step
// Source impedance: ~10 kΩ (Thevenin from R-2R)
//
// Pin locations (UM1724 → "ST Morpho / Arduino connector"):
//   PB0  CN8 pin A3  (Arduino analog A3)  ← D0 LSB
//   PB4  CN10 pin 28
//   PB5  CN10 pin 30
//   PB6  CN10 pin 17                      ← D3 MSB
//

#include "dac.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOB_MODER (*((volatile uint32_t *)0x40020400U))
#define GPIOB_ODR (*((volatile uint32_t *)0x40020414U))

// DAC uses PB0(D0/LSB), PB4(D1), PB5(D2), PB6(D3/MSB) — non-consecutive
// PB3 is SYS_JTDO-SWO on Nucleo-F446RE and cannot be used as GPIO
#define DAC_MASK ((1U << 0) | (1U << 4) | (1U << 5) | (1U << 6))

void DAC_Init(void) {
  RCC_AHB1ENR |= (1U << 1); // enable GPIOB clock
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // Set PB0, PB4, PB5, PB6 → General-purpose output (MODER = 01)
  // MODER register: 2 bits per pin
  //   PB0: MODER[1:0]   PB4: MODER[9:8]   PB5: MODER[11:10]   PB6: MODER[13:12]
  GPIOB_MODER &= ~((3U << 0) | (3U << 8) | (3U << 10) | (3U << 12)); // clear
  GPIOB_MODER |=
      ((1U << 0) | (1U << 8) | (1U << 10) | (1U << 12)); // set output

  GPIOB_ODR &= ~DAC_MASK; // initialize all DAC pins to 0 (silent)
}

// DAC_Out: write 4-bit value (0–15) to PB0/PB4/PB5/PB6
// Pins are non-consecutive — each bit is mapped individually:
//   data bit0 (LSB) → PB0     data bit1 → PB4
//   data bit2       → PB5     data bit3 (MSB) → PB6
void DAC_Out(uint8_t data) {
  uint32_t odr = GPIOB_ODR & ~DAC_MASK;
  if (data & 0x01U)
    odr |= (1U << 0); // bit0 (LSB) → PB0
  if (data & 0x02U)
    odr |= (1U << 4); // bit1       → PB4
  if (data & 0x04U)
    odr |= (1U << 5); // bit2       → PB5
  if (data & 0x08U)
    odr |= (1U << 6); // bit3 (MSB) → PB6
  GPIOB_ODR = odr;
}
