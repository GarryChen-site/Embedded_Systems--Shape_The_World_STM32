// ssd1306.h — SSD1306 OLED 128×64 Driver via I2C1 (Bare-Metal)
//
// Hardware (I2C1):
//   PB6 (SCL)  CN10 pin 17
//   PB7 (SDA)  CN10 pin 19
//   3.3V       VCC
//   GND        GND
//
// Screen geometry: 128 columns x 64 rows (8 pages of 8 pixels tall)
// Character grid: 21 characters across x 8 rows tall (using 6x8 ASCII font)

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SCREENW   128
#define SCREENH    64
#define SSD1306_I2C_ADDR   0x3C

// Initialize I2C1 and SSD1306 display
void SSD1306_Init(void);

// Clear the display and reset cursor to (0,0)
void SSD1306_Clear(void);

// Set character cursor (col: 0..20, row: 0..7)
void SSD1306_SetCursor(uint8_t col, uint8_t row);

// Print a character at cursor position
void SSD1306_OutChar(char c);

// Print a null-terminated string
void SSD1306_OutString(const char *str);

// Print unsigned decimal number
void SSD1306_OutUDec(uint16_t n);

// Send frame buffer to OLED display
void SSD1306_DisplayBuffer(void);

// Clear RAM buffer
void SSD1306_ClearBuffer(void);

#endif // SSD1306_H
