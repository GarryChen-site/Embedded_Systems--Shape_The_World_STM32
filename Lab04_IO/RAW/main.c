// main.c
// Lab 4 IO — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX (Tiva LaunchPad)
// Ported to: STM32 F446RE Nucleo board
//
// -------------------------------------------------------------------------
// ORIGINAL BEHAVIOR (TM4C123GX):
//   SW1 (PF4) and SW2 (PF0) — both active LOW, internal pull-up
//   Both pressed   → LED blue
//   SW1 only       → LED red
//   SW2 only       → LED green
//   Neither        → LED off
//
// ADAPTED BEHAVIOR (STM32 F446RE — single green LED LD2 on PA5):
//   The Nucleo has only ONE built-in button (B1, PC13) and ONE LED (LD2, PA5).
//   A second button (SW2) must be wired externally to PC12 (see wiring below).
//
//   LED state is encoded as a blink PATTERN to distinguish all 4 cases:
//   Both pressed   → LED SOLID ON             (= blue on TM4C)
//   SW1 only       → FAST blink (200ms)       (= red on TM4C)
//   SW2 only       → SLOW blink (800ms)       (= green on TM4C)
//   Neither        → LED SOLID OFF            (= off on TM4C)
//
// -------------------------------------------------------------------------
// HARDWARE — STM32 F446RE NUCLEO:
//   LD2  (User LED, green, active HIGH) → PA5
//
//   B1   (Built-in button, SW1) → PC13, active LOW
//        External 4.7 kΩ pull-up on board — no internal pull-up needed
//
//   SW2  (External button)      → PC12, active LOW
//        Wire: one side to PC12, other side to GND
//        Internal pull-up enabled in software (GPIOC_PUPDR)
//        PC12 pin location: Nucleo CN10 connector
//        Source: UM1724, chapter "Hardware layout and configuration" → "ST
//        morpho connector" → CN10 table
//
//
// -------------------------------------------------------------------------

#include <stdint.h>

// RCC - Reset and Clock Control
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))

// RCC_AHB1ENR bit definitions
#define GPIOAEN (1U << 0) // Bit 0: GPIOA clock enable
#define GPIOCEN (1U << 2) // Bit 2: GPIOC clock enable

// GPIOA - PA5 is the user LED(LD2, green, actibe HIGH)
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

// PA5 bit fields:
// MODER bits [11:10]:  00=Input  01=Output  10=AF  11=Analog
#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10) // 01 = output
#define PA5_ODR (1U << 5)        // ODR bit 5 = PA5

// GPIOC — PC13 (SW1, built-in B1) and PC12 (SW2, external)
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

// PC13 (SW1 — built-in B1):
// MODER bits [27:26]:  00 = input
// PUPDR bits [27:26]:  00 = none
// IDR  bit  13:        reads current pin level
#define PC13_MODER_MASK (3U << 26)
#define PC13_PUPDR_MASK (3U << 26)
#define SW1_IDR (1U << 13) // 0 when pressed (active LOW)

// PC12 (SW2 — external button):
// MODER bits [25:24]:  00 = input
// PUPDR bits [25:24]:  01 = pull-up (internal, since no external R)
// IDR  bit  12:        reads current pin level
#define PC12_MODER_MASK (3U << 24)
#define PC12_PUPDR_MASK (3U << 24)
#define PC12_PUPDR_UP (1U << 24) // 01 = pull-up
#define SW2_IDR (1U << 12)       // 0 when pressed (active LOW)

#define DELAY_200MS 800000UL
#define DELAY_800MS 3200000UL

void GPIO_Init(void);
void Delay(volatile uint32_t count);

int main(void) {
  GPIO_Init();

  while (1) {

    // Read both switches
    // Both buttons: active LOW (0 = pressed, 1 = released)
    uint32_t sw1_pressed = !(GPIOC_IDR & SW1_IDR);
    uint32_t sw2_pressed = !(GPIOC_IDR & SW2_IDR);

    // Apply LED state on switch combination
    if (sw1_pressed && sw2_pressed) {
      // BOTH pressed -> SOLID ON
      GPIOA_ODR |= PA5_ODR;

    } else if (sw1_pressed && !sw2_pressed) {
      // SW1 only -> FAST BLINK 200ms
      GPIOA_ODR |= PA5_ODR; // ON
      Delay(DELAY_200MS);
      GPIOA_ODR &= ~PA5_ODR; // OFF
      Delay(DELAY_200MS);

    } else if (!sw1_pressed && sw2_pressed) {
      // SW2 only -> SLOW BLINK 800ms
      GPIOA_ODR |= PA5_ODR; // ON
      Delay(DELAY_800MS);
      GPIOA_ODR &= ~PA5_ODR; // OFF
      Delay(DELAY_800MS);

    } else {
      // NEITHER pressed -> SOLID OFF
      GPIOA_ODR &= ~PA5_ODR;
    }
  }
}

// =========================================================================
// GPIO_Init
// Configure PA5 as output (LED) and PC13/PC12 as inputs (SW1, SW2).
// =========================================================================
void GPIO_Init(void) {

  // -----------------------------------------------------------------
  // STEP 1: Enable clocks for GPIOA (LED) and GPIOC (both buttons)
  // -----------------------------------------------------------------
  //
  // Register: RCC_AHB1ENR
  //   Bit 0 (GPIOAEN) = 1 → GPIOA clock on
  //   Bit 2 (GPIOCEN) = 1 → GPIOC clock on
  //   Both SW1 (PC13) and SW2 (PC12) share the same GPIOC block,
  //   so one clock-enable covers both.
  // -----------------------------------------------------------------
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);

  // -----------------------------------------------------------------
  // STEP 2: Clock settle delay
  // -----------------------------------------------------------------
  // RM0390 §6.3.10 Note: read-back after enabling clock to stall pipeline
  // -----------------------------------------------------------------
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // -----------------------------------------------------------------
  // STEP 3: Configure PA5 as general-purpose output (LED)
  //
  // GPIOA_MODER bits [11:10] for PA5:  → 01 (output)
  // Pattern: clear first (→ 00), then set (→ 01)
  // -----------------------------------------------------------------
  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  // -----------------------------------------------------------------
  // STEP 4: Configure PC13 as input (SW1 — built-in B1)
  //
  // GPIOC_MODER bits [27:26] for PC13: → 00 (input)
  // 00 is the reset value; we clear explicitly for safety.
  // -----------------------------------------------------------------
  GPIOC_MODER &= ~PC13_MODER_MASK;

  // -----------------------------------------------------------------
  // STEP 5: Configure PC12 as input (SW2 — external)
  // -----------------------------------------------------------------
  // GPIOC_MODER bits [25:24] for PC12: → 00 (input)
  // -----------------------------------------------------------------
  GPIOC_MODER &= ~PC12_MODER_MASK;

  // -----------------------------------------------------------------
  // STEP 6: Pull-up configuration
  // -----------------------------------------------------------------
  //
  // PC13 (SW1): board has external 4.7 kΩ pull-up
  // disable internal pull: PUPDR[27:26] = 00  (no internal pull)
  //
  // PC12 (SW2): external button, NO external pull-up resistor
  // enable internal pull-up: PUPDR[25:24] = 01
  //
  // Register: GPIOC_PUPDR
  //   00 = no pull, 01 = pull-up, 10 = pull-down
  // -----------------------------------------------------------------
  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
  GPIOC_PUPDR &= ~PC12_PUPDR_MASK;
  GPIOC_PUPDR |= PC12_PUPDR_UP;
}

void Delay(volatile uint32_t count) {
  while (count > 0) {
    count--;
  }
}