// main.c
// Lab 8 SwitchLEDInterface — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX — EXTERNAL switch on PE0, EXTERNAL LED on PE1
// Ported to: STM32 F446RE Nucleo board
//
// -------------------------------------------------------------------------
// BEHAVIOR (identical to original):
//
//   1. LED starts ON
//   2. Wait 100 ms
//   3. If switch is pressed  (PE0 = 1 original / PC12 = 1 here) → TOGGLE LED
//      If switch is released (PE0 = 0 original / PC12 = 0 here) → LED ON
//   4. Repeat 2–3 forever
//
// SWITCH POLARITY — important difference from Labs 6/7:
//   Labs 6/7: switch active LOW  (pressed=0, Nucleo board pulls up to 3.3V)
//   Lab  8:   switch active HIGH (pressed=1, external pull-DOWN, switch
//   to 3.3V) The original PE0 had NO internal pull-up — an external pull-down
//   to GND. We replicate this on PC12 using the MCU's internal pull-down
//   (PUPDR=10).
//
// -------------------------------------------------------------------------
// HARDWARE — ALL EXTERNAL (the point of this lab):
//
//   SW  (External switch input) → PC12, active HIGH
//       Wiring:
//         Switch leg A → PC12  (CN10 pin 12)
//         Switch leg B → 3.3V  (CN7  pin 16)
//         Internal pull-down enabled via PUPDR[25:24]=10 → PC12 = LOW when open
//
//   LED (External LED output)   → PA6, active HIGH
//       Wiring:
//         PA6 (CN10 pin 13) → 330 Ω → LED anode (+) → LED cathode (−) → GND
//         GND: CN10 pin 9
//       Why 330 Ω: (3.3V − 2.0V) / 4mA ≈ 325 Ω → use 330 Ω
//
// WHY PC12 AND PA6?
//   PC12: not reserved (not PA2/PA3/PA13/PA14/PB3), accessible on CN10 pin 12
//         supports internal pull-down
//   PA6:  not reserved, accessible on CN10 pin 13, used for LED in Lab 7 too
// -------------------------------------------------------------------------

#include <stdint.h>

// =========================================================================
// REGISTER DEFINITIONS
// =========================================================================

// RCC_AHB1ENR — GPIO clock enables
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOAEN (1U << 0) // GPIOA clock enable
#define GPIOCEN (1U << 2) // GPIOC clock enable

// -------------------------------------------------------------------------
// GPIOA — PA6 = external LED output (active HIGH)
// -------------------------------------------------------------------------
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

// PA6: MODER bits [13:12] = 01 (output)  (RM0390 §8.4.1)
#define PA6_MODER_MASK (3U << 12)
#define PA6_MODER_OUT (1U << 12)
#define PA6_ODR (1U << 6) // ODR bit 6 = PA6  (RM0390 §8.4.6)

// -------------------------------------------------------------------------
// GPIOC — PC12 = external switch input (active HIGH, internal pull-down)
// -------------------------------------------------------------------------
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

// PC12: MODER bits [25:24] = 00 (input)
#define PC12_MODER_MASK (3U << 24)

// PC12: PUPDR bits [25:24] = 10 (pull-DOWN)
//   10 = pull-down: PC12 reads 0 when switch is open (floating → pulled to GND)
//        PC12 reads 1 when switch is closed (switch connects PC12 to 3.3V)
#define PC12_PUPDR_MASK (3U << 24)
#define PC12_PUPDR_PD (2U << 24) // 10 = pull-down
#define PC12_IDR (1U << 12)      // IDR bit 12 = PC12

// =========================================================================
// DELAY CONSTANT
// =========================================================================
// Clock: 16 MHz HSI default  (RM0390 §6.2.2)
// GCC -O0: ~4 cycles per inner loop iteration
// CYCLE_ONE_MS = 16,000,000 / 4 / 1000 = 4,000
//
// TM4C original: CYCLE_ONE_MS = 16000 × 25/30 ≈ 13,333  (80 MHz, ~6 cyc/iter)
#define CYCLE_ONE_MS 4000UL

// =========================================================================
// FUNCTION PROTOTYPES
// =========================================================================
void GPIO_Init(void);         // replaces portE_init()
void Delay1ms(uint32_t time); // identical signature

// =========================================================================
// MAIN (identical structure to original)
// =========================================================================
int main(void) {
  GPIO_Init();

  // LED starts ON
  GPIOA_ODR |= PA6_ODR; // PA6 HIGH → LED ON

  while (1) {
    Delay1ms(100); // wait 100ms

    // Read switch state
    uint32_t input = GPIOC_IDR & PC12_IDR;

    if (input != 0) {
      // Switch pressed (PC12 = 1) → toggle LED
      GPIOA_ODR ^= PA6_ODR; // XOR toggles PA6
    } else {
      // Switch open (PC12 = 0) → LED ON
      GPIOA_ODR |= PA6_ODR; // PA6 HIGH → LED ON
    }
  }
}

// =========================================================================
// GPIO_Init
// Configure PA6 as output (LED), PC12 as input with pull-down (switch).
// Mirrors portE_init() from the original lab.
// =========================================================================
void GPIO_Init(void) {

  // Enable clocks
  // STM32: GPIOA (PA6 LED) + GPIOC (PC12 switch)
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy; // settle

  // ---- PA6: output (external LED) ----
  GPIOA_MODER &= ~PA6_MODER_MASK; // clear
  GPIOA_MODER |= PA6_MODER_OUT;   // 01 = output

  // ---- PC12: input with internal pull-DOWN (external switch, active HIGH)
  // STM32: MODER=00 (input), PUPDR=10 (pull-down)
  GPIOC_MODER &= ~PC12_MODER_MASK; // 00 = input

  GPIOC_PUPDR &= ~PC12_PUPDR_MASK; // clear pull config
  GPIOC_PUPDR |= PC12_PUPDR_PD;    // 10 = pull-DOWN
  // Pull-down keeps PC12 LOW when switch is open → avoids floating input noise
  // Switch connects PC12 to 3.3V → PC12 goes HIGH when pressed
}

// =========================================================================
// Delay1ms
// Busy-wait. Identical structure to original TM4C Delay1ms.
// CYCLE_ONE_MS recalculated for 16 MHz / ~4 cycles per loop.
// =========================================================================
void Delay1ms(uint32_t time) {
  volatile uint32_t i;
  while (time > 0) {
    i = CYCLE_ONE_MS;
    while (i > 0) {
      i--;
    }
    time--;
  }
}
