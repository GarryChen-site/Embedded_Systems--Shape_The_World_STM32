// main.c
// Lab 6 BranchingFunctionsDelays — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX (Tiva LaunchPad)
// Ported to: STM32 F446RE Nucleo board
//
// -------------------------------------------------------------------------
// ORIGINAL BEHAVIOR (TM4C123GX):
//   SW1 (PF4, active LOW) NOT pressed → LED (PF2, blue) stays ON
//   SW1 (PF4, active LOW)     pressed → LED (PF2, blue) TOGGLES every 100ms
//   LED starts ON at power-on (PF2=1 → blue)
//
// ADAPTED BEHAVIOR (STM32 F446RE — single green LED LD2 on PA5):
//   B1 (PC13, active LOW) NOT pressed → LD2 stays ON
//   B1 (PC13, active LOW)     pressed → LD2 TOGGLES every 100ms
//   LED starts ON at power-on
//
// -------------------------------------------------------------------------
// LOGIC ANALYZER → OSCILLOSCOPE:
//   The original lab uses Keil uVision's software Logic Analyzer
//   to observe the digital waveform on PF2 in simulation.
//   On the real STM32 Nucleo board, you observe PA5 directly:
//   Connect oscilloscope probe to CN10 pin 11 (PA5 / Morpho)
//
// -------------------------------------------------------------------------
// HARDWARE (STM32 F446RE Nucleo):
//   LD2  (User LED, green, active HIGH) → PA5
//   B1   (User button, active LOW)      → PC13
//
// -------------------------------------------------------------------------

#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOAEN (1U << 0) // GPIOA clock enable
#define GPIOCEN (1U << 2) // GPIOC clock enable

#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10) // 01 = output
#define PA5_ODR (1U << 5)        // ODR bit 5 = PA5

#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

#define PC13_MODER_MASK (3U << 26)
#define PC13_PUPDR_MASK (3U << 26)
#define PC13_IDR (1U << 13)

#define DELAY_100MS 400000UL

void GPIO_Init(void);
void Delay100ms(uint32_t count);

// =========================================================================
// GLOBAL VARIABLES
// (kept as globals to mirror original lab — observable in
// debugger/oscilloscope)
// =========================================================================
uint32_t input;  // button state: 0 = pressed, non-zero = not pressed
uint32_t output; // LED state:    1 = ON,       0 = OFF

int main(void) {
  GPIO_Init();

  // LED starts ON
  GPIOA_ODR |= PA5_ODR;

  while (1) {
    Delay100ms(1);

    // Read button
    // PC13 is active LOW: 0 = pressed, 1 = released
    input = GPIOC_IDR & PC13_IDR;

    if (input == 0x00) {
      // SW1 pressed → TOGGLE LED
      GPIOA_ODR ^= PA5_ODR;
    } else {
      // SW1 not pressed → LED stays ON
      GPIOA_ODR |= PA5_ODR;
    }

    // capture LED state for debugger inspection
    output = (GPIOA_ODR >> 5) & 1U;
  }
}

// =========================================================================
// GPIO_Init
// Configure PA5 as output (LED) and PC13 as input (button).
// =========================================================================
void GPIO_Init(void) {

  // Enable GPIOA and GPIOC clocks
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // PA5 → general-purpose output (LED)
  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  // PC13 → general-purpose input (button)
  GPIOC_MODER &= ~PC13_MODER_MASK;

  // No internal pull-up on PC13 — Nucleo has external 4.7 kΩ pull-up
  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
}

// =========================================================================
// Delay100ms
// Busy-wait delay. Each call to Delay100ms(1) waits approximately 100 ms.
// =========================================================================
void Delay100ms(uint32_t time) {
  volatile uint32_t i;
  while (time > 0) {
    i = DELAY_100MS;
    while (i > 0) {
      i--;
    }
    time--;
  }
}