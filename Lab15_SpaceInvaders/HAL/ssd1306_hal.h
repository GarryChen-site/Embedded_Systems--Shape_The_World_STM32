// ssd1306_hal.h — SSD1306 OLED 128×64 driver (HAL version), Lab 15
//
// HAL port of ssd1306.h.  Provides an identical API to the bare-metal
// version so SpaceInvaders.c needs no changes to its display calls.
//
// HARDWARE (I2C1, 400 kHz):
//   PB6  SCL  CN10 pin 17  (Arduino D10)
//   PB7  SDA  CN10 pin 19
//   3.3V  VCC  CN8 pin 4
//   GND   GND
//
// I2C ADDRESS:
//   0x3C  (SA0 = GND, default)  → HAL DevAddress = 0x78 (0x3C << 1)
//
// SCREEN GEOMETRY:
//   SCREENW = 128 columns, SCREENH = 64 rows
//   8 pages of 8 rows;  ScreenBuffer[page*128 + col], bit[row%8] = pixel

#ifndef SSD1306_HAL_H
#define SSD1306_HAL_H
#include <stdint.h>

// Screen geometry — identical to Nokia5110 replacements in ssd1306.h
#define SCREENW    128
#define SCREENH     64
#define MAX_X      128
#define MAX_Y       64

// 7-bit I2C address (SA0=GND default).  Change to 0x3D if SA0=VCC.
#define SSD1306_I2C_ADDR_7   0x3C
// HAL DevAddress (8-bit form = 7-bit << 1)
#define SSD1306_I2C_ADDR     ((uint16_t)(SSD1306_I2C_ADDR_7 << 1))  // 0x78

// 1024-byte frame buffer (128 columns × 8 pages)
extern uint8_t ScreenBuffer[SCREENW * (SCREENH / 8)];

// -------------------------------------------------------------------------
// API — drop-in replacement for ssd1306.h used in SpaceInvaders.c
// -------------------------------------------------------------------------

void SSD1306_Init(void);       // I2C1 already init by CubeIDE; sends init cmds
void SSD1306_ClearBuffer(void); // zero the frame buffer (no display update)
void SSD1306_DisplayBuffer(void); // push ScreenBuffer to OLED via I2C

// Draw a 4-bit LM3S BMP sprite into ScreenBuffer
void SSD1306_PrintBMP(uint8_t xpos, uint8_t ypos,
                      const uint8_t *ptr, uint8_t threshold);

void SSD1306_Clear(void);                       // clear buffer + display
void SSD1306_SetCursor(uint8_t x, uint8_t y);  // char col (0-21), page (0-7)
void SSD1306_OutChar(char c);
void SSD1306_OutString(const char *str);
void SSD1306_OutUDec(uint16_t n);

#endif /* SSD1306_HAL_H */
