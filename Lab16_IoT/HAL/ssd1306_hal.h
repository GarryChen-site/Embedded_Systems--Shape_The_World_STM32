// ssd1306_hal.h — SSD1306 OLED 128×64 driver (HAL version), Lab 16 IoT
//
// HAL port of ssd1306.h. Provides an identical API to the bare-metal
// version so application code needs no changes to its display calls.
//
// HARDWARE (I2C1, 400 kHz Fast Mode):
//   PB6 (SCL)  CN10 pin 17  (or PB8 CN10 pin 3 / Arduino D15 if remapped)
//   PB7 (SDA)  CN10 pin 19  (or PB9 CN10 pin 5 / Arduino D14 if remapped)
//   3.3V       VCC
//   GND        GND
//
// I2C ADDRESS:
//   0x3C  (SA0 = GND, default)  -> HAL DevAddress = 0x78 (0x3C << 1)
//
// SCREEN GEOMETRY:
//   SCREENW = 128 columns, SCREENH = 64 rows
//   8 pages of 8 rows; ScreenBuffer[page*128 + col], bit[row%8] = pixel

#ifndef SSD1306_HAL_H
#define SSD1306_HAL_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define SCREENW    128
#define SCREENH     64
#define MAX_X      128
#define MAX_Y       64

// 7-bit I2C address (SA0=GND default). Change to 0x3D if SA0=VCC.
#define SSD1306_I2C_ADDR_7   0x3C
// HAL DevAddress (8-bit form = 7-bit << 1)
#define SSD1306_I2C_ADDR     ((uint16_t)(SSD1306_I2C_ADDR_7 << 1))  // 0x78

// 1024-byte frame buffer (128 columns x 8 pages)
extern uint8_t ScreenBuffer[SCREENW * (SCREENH / 8)];

// Initialize display (I2C1 must already be initialized by MX_I2C1_Init)
void SSD1306_Init(void);

// Clear the frame buffer in RAM (no display update)
void SSD1306_ClearBuffer(void);

// Push ScreenBuffer to OLED via HAL_I2C_Master_Transmit
void SSD1306_DisplayBuffer(void);

// Clear buffer and update display, reset cursor to (0,0)
void SSD1306_Clear(void);

// Set character cursor: col (0..20), page (0..7)
void SSD1306_SetCursor(uint8_t col, uint8_t row);

// Print a single character at cursor position
void SSD1306_OutChar(char c);

// Print a null-terminated string at cursor position
void SSD1306_OutString(const char *str);

// Print an unsigned decimal number
void SSD1306_OutUDec(uint16_t n);

#endif /* SSD1306_HAL_H */
