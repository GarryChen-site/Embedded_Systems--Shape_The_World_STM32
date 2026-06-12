
#include "adc.h"
#include "measure.h"
#include "uart.h"
#include <stdint.h>

#define STK_CTRL (*((volatile unsigned int *)0xE000E010))
#define STK_LOAD (*((volatile unsigned int *)0xE000E014))
#define STK_VAL (*((volatile unsigned int *)0xE000E018))
#define SCB_SHPR3 (*((volatile unsigned int *)0xE000ED20))

volatile uint32_t ADCdata = 0; // latest 12-bit ADC sample
volatile uint32_t Angle = 0;   // converted angle in 0.1° units (0–2400)
volatile uint32_t Flag = 0;    // 1 = new data in mailbox, 0 = consumed

static void SysTick_Init(uint32_t period) {
  STK_CTRL = 0;
  STK_VAL = 0;
  STK_LOAD = period - 1;

  SCB_SHPR3 = (SCB_SHPR3 & 0x00FFFFFF) | (2 << 29);

  STK_CTRL = 0x07;
}

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

  UART2_OutString("Lab14 ADC — Trimmer Angle Meter\r\n");
  UART2_OutString("Format: DDD.D deg  (0.0 = CCW, 240.0 = CW)\r\n");
  UART2_OutString("---\r\n");
  while (1) {
    if (Flag) {
      Flag = 0;
      UART2_OutString(String);
      UART2_OutString("\r\n");
    }
  }
}