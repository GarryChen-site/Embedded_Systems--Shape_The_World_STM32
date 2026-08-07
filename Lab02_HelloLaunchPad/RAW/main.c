// Lab 2 HelloLaunchPad — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX (Tiva LaunchPad)
// Ported to: STM32 F446RE Nucleo board
//
// -------------------------------------------------------------------------
// ORIGINAL BEHAVIOR (TM4C123GX):
//   SW1 (PF4, active LOW) NOT pressed  → LED toggles blue  ↔ red   (100ms each)
//   SW1 (PF4, active LOW)     pressed  → LED toggles blue  ↔ green  (100ms
//   each)
//
// ADAPTED BEHAVIOR (STM32 F446RE — single green LED LD2 on PA5):
//   B1 (PC13, active LOW) NOT pressed  → LED blinks FAST (100ms ON, 100ms OFF)
//                                         simulates blue ↔ red
//   B1 (PC13, active LOW)     pressed  → LED blinks SLOW (100ms ON, 400ms OFF)
//                                         simulates blue ↔ green
//
// ADAPTATION REASON:
//   The Nucleo board has only one user LED (LD2, green, PA5).
//   The TM4C LaunchPad had an RGB LED on PF1/PF2/PF3.
//   Blink speed is used to preserve the two distinguishable states.
// -------------------------------------------------------------------------

#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))

#define GPIOAEN (1U << 0)
#define GPIOCEN (1U << 2)

#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10)
#define PA5_ODR (1U << 5)

#define PC13_MODER_MASK (3U << 26)
#define PC13_PUPDR_MASK (3U << 26)
#define PC13_IDR (1U << 13)

#define DELAY_100MS 400000UL
#define DELAY_400MS 1600000UL

void GPIO_Init(void);
void Delay(volatile uint32_t count);

int main(void) {
  GPIO_Init();

  while (1) {
    uint32_t button_pressed = !(GPIOC_IDR & PC13_IDR);

    GPIOA_ODR |= PA5_ODR;
    Delay(DELAY_100MS);

    GPIOA_ODR &= ~PA5_ODR;

    if (button_pressed) {
      Delay(DELAY_400MS);
    } else {
      Delay(DELAY_100MS);
    }
  }
}

void GPIO_Init(void) {
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);

  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  GPIOC_MODER &= ~PC13_MODER_MASK;

  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
}

void Delay(volatile uint32_t count) {
  while (count) {
    count--;
  }
}