
#include "adc.h"
#include "measure.h"
#include "uart.h"
#include <stdint.h>

#define STK_CTRL (*((volatile unsigned int *)0xE000E010))
#define STK_LOAD (*((volatile unsigned int *)0xE000E014))
#define STK_VAL (*((volatile unsigned int *)0xE000E018))
#define SCB_SHPR3 (*((volatile unsigned int *)0xE000ED20))

volatile uint32_t ADCdata = 0;
volatile uint32_t Angle = 0;
volatile uint32_t Flag = 0;

static void SysTick_Init(uint32_t period) {}

void SysTick_Handler(void) {
  ADCdata = ADC0_In();
  Angle = Convert(ADCdata);
  ConvertToString(Angle);
  Flag = 1;
}

int main(void) {
  ADC0_Init();
  UART2_Init();
  SysTick_Init(400000); // 16,000,000 / 40 Hz = 400,000

  while (1) {
    if (Flag) {
      Flag = 0;
      UART2_OutString(String);
      UART2_OutString("\r\n");
    }
  }
}