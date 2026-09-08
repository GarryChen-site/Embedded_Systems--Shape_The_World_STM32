// adc.c — Lab 14 ADC, STM32 F446RE port
//
// Original: ADC0 SS3 on PE2 / AIN1 (TM4C123)
// Ported to: ADC1 CH0  on PA0 / AIN0 (STM32 F446RE)
//
// PIN WIRING (Square Trimming Potentiometer — 3 leads):
//
//   Pot pin 1 (CCW end) ─── GND
//   Pot pin 2 (wiper)   ─── PA0  (CN8 pin 1 "A0")
//   Pot pin 3 (CW end)  ─── 3.3V (CN8 pin 4)
//
// The wiper sweeps the ADJUSTMENT ANGLE (240°) of the 270° mechanical range.
// At 0°  → wiper = GND  → VA0 = 0.000 V → ADC = 0
// At 240° → wiper = 3.3V → VA0 = 3.300 V → ADC = 4095
//
// ADC CONFIGURATION :
//   - ADC1, channel 0 (PA0 = AIN0)
//   - Resolution: 12-bit (0–4095)
//   - Trigger: software (SWSTART)
//   - Alignment: right
//   - Sampling time: 480 cycles (maximum, best accuracy for high-Z pot)
//   - Single conversion mode (CONT=0)
//
// Why PA0?
//   - PA0 = ADC1_IN0, confirmed available (no ST-LINK conflict, not a boot pin)
//   - Accessible on CN8 pin 1 (Arduino "A0")
//   - PA2/PA3 reserved for USART2/VCP — must not use those for ADC
//

#include "adc.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define RCC_APB2ENR (*((volatile uint32_t *)0x40023844U))

#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_PUPDR (*((volatile uint32_t *)0x4002000CU))

#define ADC1_SR (*((volatile uint32_t *)0x40012000U))
#define ADC1_CR1 (*((volatile uint32_t *)0x40012004U))
#define ADC1_CR2 (*((volatile uint32_t *)0x40012008U))
#define ADC1_SMPR2 (*((volatile uint32_t *)0x40012010U))
#define ADC1_SQR1 (*((volatile uint32_t *)0x4001202CU)) // sequence reg 1
#define ADC1_SQR3 (*((volatile uint32_t *)0x40012034U)) // sequence reg 3
#define ADC1_DR (*((volatile uint32_t *)0x4001204CU))

#define ADC_SR_EOC (1U << 1)
#define ADC_CR2_ADON (1U << 0)
#define ADC_CR2_SWSTART (1U << 30)

void ADC0_Init(void) {

  // 1. Enable GPIOA clock
  RCC_AHB1ENR |= (1U << 0);
  volatile uint32_t delay = RCC_AHB1ENR;
  (void)delay;

  // 2. Set PA0 to analog mode (MODER[1:0] = 11)
  GPIOA_MODER |= (3U << 0);

  // 3. Set PA0 to no pull-up, no pull-down (PUPDR[1:0] = 00)
  GPIOA_PUPDR &= ~(3 << 0);

  // 4. Enable ADC1 clock
  RCC_APB2ENR |= (1 << 8);
  delay = RCC_AHB1ENR;
  (void)delay;

  // 5. Power off ADC before configuring
  ADC1_CR2 = 0;

  // 6. CR1: 12-bit resolution, no scan, no interrupt
  ADC1_CR1 = 0;

  // 7. CR2: right-aligned, software trigger, single conversion
  ADC1_CR2 = 0;

  // 8. Sampling time for CH0: 480 cycles
  ADC1_SMPR2 |= (7U << 0);

  // 9. Sequence: 1 conversion, first channel = CH0
  ADC1_SQR1 &= ~(0xFU << 20);

  ADC1_SQR3 &= ~(0x1FU << 0);

  // 10. Power on ADC
  ADC1_CR2 |= ADC_CR2_ADON;
  // Delay ≥ 3 µs at 16 MHz ≈ 48 cycles — spin briefly
  for (volatile uint32_t i = 0; i < 100U; i++)
    ;
}

// -------------------------------------------------------------------------
// ADC0_In — software-triggered, busy-wait, 12-bit result
// Identical 4-step pattern to original TM4C:
//   1. Initiate   2. Wait   3. Read   4. Clear flag
// -------------------------------------------------------------------------
uint32_t ADC0_In(void) {

  // 1. Start conversion (SWSTART)
  ADC1_CR2 |= ADC_CR2_SWSTART;

  // 2. Wait for End-Of-Conversion flag (EOC)
  while (!(ADC1_SR & ADC_SR_EOC))
    ;

  // 3. Read result (12-bit)
  uint32_t res = ADC1_DR & 0xFFFU;

  // 4. Clear EOC (reading DR clears it automatically on STM32 — no extra step)
  return res;
}