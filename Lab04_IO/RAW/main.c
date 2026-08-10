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

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))

#define GPIOAEN (1U << 0)
#define GPIOCEN (1U << 2)

#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

// PA5 bit fields:
// MODER bits [11:10]:  00=Input  01=Output  10=AF  11=Analog
#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10) // 01 = output
#define PA5_ODR (1U << 5)        // ODR bit 5 = PA5

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

void delay_ms(volatile unsigned int ms) {
  volatile unsigned int count = ms * 1600;
  while (count > 0) {
    count--;
  }
}

void GPIO_Init(void) {
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);

  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  GPIOC_MODER &= ~PC13_MODER_MASK;

  GPIOC_MODER &= ~PC12_MODER_MASK;

  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
  GPIOC_PUPDR &= ~PC12_PUPDR_MASK;
  GPIOC_PUPDR |= PC12_PUPDR_UP;
}

int main(void) {
  GPIO_Init();

  while (1) {

    unsigned sw1_pressed = !(GPIOC_IDR & SW1_IDR);
    unsigned sw2_pressed = !(GPIOC_IDR & SW2_IDR);

    if (sw1_pressed && sw2_pressed) {
      GPIOA_ODR |= PA5_ODR;
    } else if (sw1_pressed && !sw2_pressed) {
      GPIOA_ODR |= PA5_ODR;
      delay_ms(100);
      GPIOA_ODR &= ~PA5_ODR;
      delay_ms(100);
    } else if (!sw1_pressed && sw2_pressed) {
      GPIOA_ODR |= PA5_ODR;
      delay_ms(400);
      GPIOA_ODR &= ~PA5_ODR;
      delay_ms(400);
    } else {
      GPIOA_ODR &= ~PA5_ODR;
    }
  }
}
