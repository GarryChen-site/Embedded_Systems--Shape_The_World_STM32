
// Pin locations
// PB3 CN9  pin 3
// PB4 CN9  pin 4
// PB5 CN9  pin 5
// PB6 CN9  pin 6

#include "dac.h"

#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define GPIOB_MODER (*((volatile unsigned int *)0x40020400))
#define GPIOB_ODR (*((volatile unsigned int *)0x40020414))

#define DAC_MASK (0xFU << 3) // bits 3..6 (PB3..PB6)

void DAC_init(void) {
  RCC_AHB1ENR |= (1U << 1);

  volatile uint32_t dummy = RCC_AHB1ENR;

  (void)dummy;

  GPIOB_MODER &= ~(0xFFU << 6); // clear MODER[13:6] (PB3..PB6)
  GPIOB_MODER |= (0x55U << 6);  // set output mode

  GPIOB_ODR &= ~DAC_MASK;
}

void DAC_Out(uint8_t data) {
  GPIOB_ODR = (GPIOB_ODR & ~DAC_MASK) | ((data & 0xFU) << 3);
}
