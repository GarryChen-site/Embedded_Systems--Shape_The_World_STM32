// piano.c — Lab 13 DAC, STM32 F446RE port
//
// Original: 4 keys on PA2–PA5 (TM4C, active HIGH, external pull-down)
// Ported to: 4 keys on PC6, PC8, PC9, PC10 (STM32, active HIGH, external
// pull-down)
//
// Wiring (each key — same as Lab 8/9 switch wiring):
//
//   3.3V rail ─── switch ─── MCU pin ───[10kΩ]─── GND
//                               ^
//                        reads 0 (open) or 1 (pressed)
//
// Pin locations (UM1724 → CN10 Morpho connector):
//   PC6   CN10 pin 4   → Key C (DO,  523 Hz)
//   PC8   CN10 pin 2   → Key D (RE,  587 Hz)
//   PC9   CN10 pin 1   → Key E (MI,  659 Hz)
//   PC10  CN10 pin 6   → Key G (SOL, 783 Hz)
//
// Priority logic: if multiple keys pressed, highest note wins
//   (same priority as original: G > E > D > C)
//

#include "piano.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

// Bit masks for PC6, PC8, PC9, PC10
#define KEY_C_BIT (1U << 6)
#define KEY_D_BIT (1U << 8)
#define KEY_E_BIT (1U << 9)
#define KEY_G_BIT (1U << 10)
#define KEY_ALL_MASK (KEY_C_BIT | KEY_D_BIT | KEY_E_BIT | KEY_G_BIT)

void Piano_Init() {
  RCC_AHB1ENR |= (1U << 2);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // Clear mode bits for PC6, PC8, PC9, PC10 (set to input)
  GPIOC_MODER &= ~(3U << 12) | // PC6
                 ~(3U << 16) | // PC8
                 ~(3U << 18) | // PC9
                 ~(3U << 20);  // PC10
}

// Piano_In — read keys and return note identifier
// Returns first match (G has highest priority if multiple pressed)
uint8_t Piano_In(void) {
  uint32_t idr = GPIOC_IDR;
  if (idr & KEY_G_BIT) { // higest frequency first
    return SOL;
  }
  if (idr & KEY_E_BIT) {
    return MI;
  }
  if (idr & KEY_D_BIT) {
    return RE;
  }
  if (idr & KEY_C_BIT) {
    return DO;
  }
  return SILENCE;
}