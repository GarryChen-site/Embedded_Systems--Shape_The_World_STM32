// ssd1306.h — Lab 14 ADC, STM32 F446RE
//
// Driver for the SSD1306 0.96" 128×64 OLED via I2C (i2c.c).
//
// DISPLAY COORDINATES:
//   col  : 0 (left) – 127 (right)
//   page : 0 (top)  –   7 (bottom)   (each page = 8 pixel rows)
//
// CHARACTER GRID (5×8 font, 1px gap between chars → 6px per char):
//   Columns of text: 128 / 6 = 21 chars
//   Rows of text   :  64 / 8 =  8 rows (= 8 pages)
//
// TYPICAL DISPLAY LAYOUT (Lab 14):
//   Page 0: header   "Lab14 ADC Meter"
//   Page 1: ruler    "---...---"
//   Page 2: raw      "ADC: 4095"
//   Page 3: (blank)
//   Page 4: label    "Angle:"
//   Page 5: value    "240.0 deg"
//   Page 6: (blank)
//   Page 7: bar      [################     ]

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

// =========================================================================
// SSD1306_Init — reset display, run init sequence, clear GDDRAM, turn on.
// Call I2C1_Init() before calling this.
// =========================================================================
void SSD1306_Init(void);

// =========================================================================
// SSD1306_Clear — zero the entire RAM framebuffer.
// Does NOT update the display — call SSD1306_Update() afterward.
// =========================================================================
void SSD1306_Clear(void);

// =========================================================================
// SSD1306_DrawString — render a null-terminated ASCII string into the
// framebuffer at the given starting column and page.
// Characters outside col 0–127 are clipped.
// Does NOT update the display — call SSD1306_Update() afterward.
// =========================================================================
void SSD1306_DrawString(uint8_t col, uint8_t page, const char *s);

// =========================================================================
// SSD1306_DrawHLine — draw a horizontal rule across the full width at
// 'page' (a thin 1-pixel line at row 0 of that page).
// Does NOT update the display.
// =========================================================================
void SSD1306_DrawHLine(uint8_t page);

// =========================================================================
// SSD1306_DrawHLine — draw a horizontal rule across the full width at
// 'page' (a thin 1-pixel line at row 0 of that page).
// Does NOT update the display.
// =========================================================================void SSD1306_DrawHLine(uint8_t page);
void SSD1306_DrawBar(uint8_t page, uint8_t percent);

// =========================================================================
// SSD1306_Update — flush the entire 1024-byte framebuffer to the SSD1306
// GDDRAM via I2C in one transaction (~23 ms at 400 kHz).
// =========================================================================
void SSD1306_Update(void);



#endif