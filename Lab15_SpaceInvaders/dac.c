

#include "dac.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOB_MODER (*((volatile uint32_t *)0x40020400U))
#define GPIOB_ODR (*((volatile uint32_t *)0x40020414U))

#define DAC_MASK (0xFU << 3) // bits 6,5,4,3

void DAC_Init(void) {
  RCC_AHB1ENR |= (1U << 1);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // PB3,PB4,PB5,PB6 → output (01)
  GPIOB_MODER &= ~(0xFFU << 6); // clear MODER[13:6] (PB3..PB6)
  GPIOB_MODER |= (0x55U << 6);  // 01 01 01 01 = output for each

  GPIOB_ODR &= ~DAC_MASK; // initialize DAC to 0 (silent)
}

// DAC_Out: write 4-bit value (0–15) to PB[6:3]
// Bit 0 of data → PB3 (LSB), Bit 3 → PB6 (MSB)
void DAC_Out(uint8_t data) {
  GPIOB_ODR = (GPIOB_ODR & ~DAC_MASK) | ((data & 0xFU) << 3);
}