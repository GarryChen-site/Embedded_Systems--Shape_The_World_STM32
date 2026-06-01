

#include "adc.h"

#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define RCC_APB2ENR (*((volatile unsigned int *)0x40023844))

#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_PUPDR (*((volatile unsigned int *)0x4002000C))