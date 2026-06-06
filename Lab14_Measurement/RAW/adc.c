

#include "adc.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define RCC_APB2ENR (*((volatile unsigned int *)0x40023844))

#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_PUPDR (*((volatile unsigned int *)0x4002000C))

#define ADC1_SR (*((volatile unsigned int *)0x40012000))
#define ADC1_CR1 (*((volatile unsigned int *)0x40012004))
#define ADC1_CR2 (*((volatile unsigned int *)0x40012008))
#define ADC1_SMPR2 (*((volatile unsigned int *)0x40012010))
#define ADC1_DR (*((volatile unsigned int *)0x4001204C))

#define ADC_SR_EOC (1u << 1)
#define ADC_CR2_SWSTART (1u << 30)

void ADC0_Init(void) {

  RCC_AHB1ENR |= (1 << 0);
  volatile unsigned int delay = RCC_AHB1ENR;
  (void)delay;

  GPIOA_MODER |= (3 << 0);

  GPIOA_PUPDR &= ~(3 << 0);

  RCC_APB2ENR |= (1 << 8);
  delay = RCC_AHB1ENR;
  (void)delay;

  ADC1_CR2 = 0;

  ADC1_CR1 = 0;

  ADC1_CR2 = 0;
}

uint32_t ADC0_In(void) {

  ADC1_CR2 |= ADC_CR2_SWSTART;

  while (!(ADC1_SR & ADC_SR_EOC))
    ;

  uint32_t res = ADC1_DR & 0xFFFU;

  return res;
}