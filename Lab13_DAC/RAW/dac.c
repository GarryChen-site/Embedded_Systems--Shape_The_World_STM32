
// Pin locations
// PB3 CN9  pin 3
// PB4 CN9  pin 4
// PB5 CN9  pin 5
// PB6 CN9  pin 6

#include "dac.h"

#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define GPIOB_MODER (*((volatile unsigned int *)0x40020400))
#define GPIOB_ODR (*((volatile unsigned int *)0x40020414))
