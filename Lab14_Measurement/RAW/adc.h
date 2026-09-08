// adc.h — Lab 14 ADC, STM32 F446RE port
//
// ADC1 channel 0, input pin PA0 (Arduino A0, CN8 pin 1)
// Software-triggered, single conversion, right-aligned 12-bit result.
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// ADC0_Init — configure ADC1 CH0 on PA0 for software-triggered sampling
// Input:  none
// Output: none
void ADC0_Init(void);

// ADC0_In — trigger one conversion and return 12-bit result (0–4095)
// Input:  none
// Output: 12-bit ADC sample (0 = 0 V, 4095 = 3.3 V)
uint32_t ADC0_In(void);



#endif