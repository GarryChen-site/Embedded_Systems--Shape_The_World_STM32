

#include "adc.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define RCC_APB2ENR (*((volatile unsigned int *)0x40023844))

#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_PUPDR (*((volatile unsigned int *)0x4002000C))

#define ADC1_SR
#define ADC1_CR2
#define ADC1_DR

#define ADC_SR_EOC
#define ADC_CR2_SWSTART

uint32_t ADC0_In(void) {

  ADC1_CR2 |= ADC_CR2_SWSTART;

  while (!(ADC1_SR & ADC_SR_EOC))
    ;

  uint32_t res = ADC1_DR & 0xFFFU;

  return res;
}