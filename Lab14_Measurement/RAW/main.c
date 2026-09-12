
#include "adc.h"
#include "i2c.h"
#include "measure.h"
#include "ssd1306.h"
#include <stdint.h>

#define STK_CTRL (*((volatile unsigned int *)0xE000E010))
#define STK_LOAD (*((volatile unsigned int *)0xE000E014))
#define STK_VAL (*((volatile unsigned int *)0xE000E018))
#define SCB_SHPR3 (*((volatile unsigned int *)0xE000ED20))

volatile uint32_t ADCdata = 0; // latest 12-bit ADC sample (0-4095)
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

// FormatADC - format 12-bit ADC value (0-4095) as "ADC: XXXX" into buf[10]
static void FormatADC(uint32_t raw, char buf[10]) {
  buf[0] = 'A';
  buf[1] = 'D';
  buf[2] = 'C';
  buf[3] = ':';
  buf[4] = ' ';
  buf[9] = '\0';
  uint32_t n = raw;
  buf[8] = (char)('0' + (n % 10U));
  n /= 10U;
  buf[7] = (char)('0' + (n % 10U));
  n /= 10U;
  buf[6] = (char)('0' + (n % 10U));
  n /= 10U;
  buf[5] = (char)('0' + (n % 10U));
}

int main(void) {
  ADC0_Init();

  I2C1_Init();

  // Brief delay to allow OLED VCC to ramp up before I2C init sequence
  for (volatile uint32_t d = 0; d < 500000U; d++)
    ;

  SSD1306_Init();
  SSD1306_DrawString(0, 0, "Lab14 ADC Meter");
  SSD1306_DrawHLine(1);
  SSD1306_DrawString(0, 4, "Angle:");
  SSD1306_Update();

  SysTick_Init(400000); // 16,000,000 / 40 Hz = 400,000

  char adc_buf[10];

  while (1) {
    if (Flag) {
      Flag = 0;

      // --- Format ADC raw value ---
      FormatADC(ADCdata, adc_buf);

      // --- Build framebuffer ---
      SSD1306_Clear();

      // Static header (re-drawn on each update to survive Clear)
      SSD1306_DrawString(0, 0, "Lab14 ADC Meter");
      SSD1306_DrawHLine(1);

      // Dynamic rows
      SSD1306_DrawString(0, 2, adc_buf);
      SSD1306_DrawString(0, 4, "Angle: ");
      SSD1306_DrawString(0, 5, String);

      // Progress bar - Angle/2400 * 100 = percent
      uint8_t pct = (uint8_t)((Angle * 100U) / 2400U);
      SSD1306_DrawBar(7, pct);

      // --- Flush buffer to display ---
      SSD1306_Update();
    }
  }
}