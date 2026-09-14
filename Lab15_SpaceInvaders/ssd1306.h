// ssd1306.h — SSD1306 OLED 128×64 driver, Lab 15 STM32 port
//
// Replaces Nokia5110.h / Nokia5110.c.
// Provides an identical API to the Nokia5110 functions used by SpaceInvaders.c
// so that the game code changes are limited to swapping header names.
//
// HARDWARE (I2C1, 400 kHz):
//   PB6  SCL  CN6 pin 7
//   PB7  SDA  CN6 pin 11
//   3.3V  VCC  CN8 pin 4
//   GND   GND
//
// I2C ADDRESS:
//   0x3C  (SA0 pin on module = GND)   ← default
//   0x3D  (SA0 pin on module = VCC)   → change SSD1306_I2C_ADDR below
//
// SCREEN GEOMETRY:
//   SCREENW = 128  columns
//   SCREENH = 64   rows
//   8 pages of 8 rows each
//   ScreenBuffer[page * 128 + col]  bit[row%8] = pixel (1=white, 0=black)
//
// BMP FORMAT (identical to Nokia5110 PrintBMP):
//   4-bit grayscale LM3S BMP with 0x76 (118) byte header.
//   xpos = column of bottom-left corner (0-127)
//   ypos = row    of bottom-left corner (0-63)
//   threshold = 4-bit color value; pixels ABOVE this value are drawn white.
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

// Screen geometry
#define SCREENW   128
#define SCREENH   64
#define MAX_X     128
#define MAX_Y     64

// I2C slave address (7-bit). Or 0x3D
#define SSD1306_I2C_ADDR 0x3C

// 1024-byte frame buffer (128 columns x 8 pages)
extern uint8_t ScreenBuffer[SCREENW * (SCREENH / 8)];

// Initialize OLED (I2C1 + SSD1306 startup sequence)
void SSD1306_Init(void);

// Clear the frame buffer in RAM (does not update display)
void SSD1306_ClearBuffer(void);

// Send ScreenBuffer to the OLED (updates display)
void SSD1306_DisplayBuffer(void);

// Draw a 4-bit LM3S BMP sprite into ScreenBuffer
// xpos: column of bottom-left corner (0 = left)
// ypos: row    of bottom-left corner (2 = near top, 63 = bottom)
// ptr:  pointer to BMP byte array
// threshold: pixels with color > threshold are drawn (use 0 for all sprites)
void SSD1306_PrintBMP(uint8_t xpos, uint8_t ypos,
                    const uint8_t* ptr, uint8_t threshold);

                   
// Clear the display and frame buffer, reset cursor to (0,0)
void SSD1306_Clear(void);

// Set character cursor position
// x: character column (0 = leftmost, each char = 6 pixels wide → 21 chars/row)
// y: page/row        (0 = top page,  each page = 8 pixels tall  →  8 rows)
void SSD1306_SetCursor(uint8_t x, uint8_t y);

// Print a null-terminated ASCII string at cursor position
void SSD1306_OutString(const char* str);

// Print a single character at cursor position, advance cursor
void SSD1306_OutChar(char c);

// Print a 16-bit unsigned decimal number (right-justified 5 digits)
void SSD1306_OutUDec(uint16_t n);
                    

#endif