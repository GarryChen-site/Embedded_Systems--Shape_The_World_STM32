// main.c — Lab 14 ADC, STM32 F446RE port
//
// Original: TM4C123, slide potentiometer (0–2 cm) on PE2/AIN1,
//           Nokia5110 LCD display, TExaS grader
//
// Ported to: STM32 F446RE, square trimming potentiometer (0°–240°) on PA0/AIN0,
//            SSD1306 0.96" OLED display via I2C (replaces Nokia5110 and UART)
//
// POTENTIOMETER WIRING:
//   Pin 1 (CCW end) → GND
//   Pin 2 (wiper)   → PA0  (CN8 pin 1 "A0")
//   Pin 3 (CW end)  → 3.3V (CN8 pin 4)
//
// OLED WIRING (SSD1306, I2C address 0x3C):
//   OLED GND → Nucleo GND
//   OLED VCC → Nucleo 3.3V  (CN6 pin 4)
//   OLED SCL → PB6  (CN10 pin 17)
//   OLED SDA → PB7  (CN10 pin 19)
//
// SYSTEM ARCHITECTURE (mailbox pattern — identical to original):
//
//   ┌────────────────────────────────────────────┐
//   │  SysTick ISR @ 40 Hz  (every 25 ms)        │
//   │    ADCdata = ADC0_In()                      │
//   │    Angle   = Convert(ADCdata)               │
//   │    ConvertToString(Angle) → String[]        │
//   │    Flag = 1  ← "mail deposited"             │
//   └────────────────────────────────────────────┘
//                        │ Flag=1
//                        ▼
//   ┌────────────────────────────────────────────┐
//   │  Main loop (foreground)                    │
//   │    if (Flag) {                             │
//   │        Flag = 0;                           │
//   │        format ADC string                   │
//   │        draw strings + bar → framebuffer    │
//   │        SSD1306_Update()  ← ~23 ms @ 400kHz│
//   │    }                                       │
//   └────────────────────────────────────────────┘
//                        │
//                        ▼ I2C1 (PB8/PB9)
//              SSD1306 0.96" OLED display
//
// DISPLAY LAYOUT (128×64, 8 text rows):
//   Page 0: "Lab14 ADC Meter"  (static header)
//   Page 1: ─────────────────  (horizontal rule)
//   Page 2: "ADC: 4095"        (raw 12-bit sample, 0–4095)
//   Page 3: (blank)
//   Page 4: "Angle:"           (static label)
//   Page 5: "240.0 deg"        (from ConvertToString — 9 chars)
//   Page 6: (blank)
//   Page 7: [############    ] (progress bar 0–240°)
//
// TIMING:
//   SysTick @ 40 Hz: ADC sample + string format every 25 ms
//   OLED update (1024 B @ 400 kHz I2C): ~23 ms per update
//   Effective display rate: ~20 Hz (one update per two ISR firings)
//
// SYSTICK @ 40 Hz:
//   RELOAD = F_CPU / F_SAMPLE = 16,000,000 / 40 = 400,000
//
#include "adc.h"
#include "i2c.h"
#include "measure.h"
#include "ssd1306.h"
#include <stdint.h>

#define STK_CTRL (*((volatile unsigned int *)0xE000E010))
#define STK_LOAD (*((volatile unsigned int *)0xE000E014))
#define STK_VAL (*((volatile unsigned int *)0xE000E018))
#define SCB_SHPR3 (*((volatile unsigned int *)0xE000ED20))

// =========================================================================
// Mailbox globals
// =========================================================================
volatile uint32_t ADCdata = 0; // latest 12-bit ADC sample (0-4095)
volatile uint32_t Angle = 0;   // converted angle in 0.1° units (0–2400)
volatile uint32_t Flag = 0;    // 1 = new data in mailbox, 0 = consumed

// =========================================================================
// SysTick_Init — configure SysTick for 40 Hz interrupts
// Input: period = STK_LOAD value
//   For 40 Hz @ 16 MHz: period = 16,000,000 / 40 = 400,000
// =========================================================================
static void SysTick_Init(uint32_t period) {
  STK_CTRL = 0;
  STK_VAL = 0;
  STK_LOAD = period - 1;

  // Priority 2 for SysTick (bits [31:24] of SHPR3)
  // Same as original: NVIC_SYS_PRI3_R = (... & 0x00FFFFFF) | 0x40000000
  SCB_SHPR3 = (SCB_SHPR3 & 0x00FFFFFF) | (2 << 29);

  STK_CTRL = 0x07;
}

// =========================================================================
// SysTick_Handler — ISR, executes every 25 ms
// Collects ADC sample, converts to angle, formats string, posts mailbox.
// =========================================================================
void SysTick_Handler(void) {
  ADCdata = ADC0_In();      // 1. read 12-bit ADC sample
  Angle = Convert(ADCdata); // 2. convert to 0.1° units
  ConvertToString(Angle);   // 3. format into String[]
  Flag = 1;                 // 4. signal main loop (mailbox)
}

// =========================================================================
// FormatADC — format 12-bit ADC value (0–4095) as "ADC: XXXX" into buf[10]
// =========================================================================
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

  // Draw static content (header + rule) that never changes
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
    // Main loop otherwise idle; all real work in SysTick ISR
  }
}