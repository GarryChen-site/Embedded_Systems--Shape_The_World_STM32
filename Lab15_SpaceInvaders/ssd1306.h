#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

// Screen geometry
// SCREENW = 128 columns
// SCREENH = 64 rows
#define SCREENW   128
#define SCREENH   64
#define MAX_X     128
#define MAX_Y     64

// I2C slave address (7-bit). Or 0x3D
#define SSD1306_I2C_ADDR 0x3C

// 1024-byte frame buffer (128 columns x 8 pages)
extern uint8_t ScreenBuffer[SCREENW * (SCREENH / 8)];

void SSD1306_Init(void);

// Clear the frame buffer in RAM (does not update display)
void SSD1306_ClearBuffer(void);

// Send ScreenBuffer to the OLED (updates display)
void SSD1306_DisplayBuffer(void);

// Draw a 4-bit LM3S BMP sprite into ScreenBuffer
void SSD1306_PrintBMP(uint8_t xpos, uint8_t ypos,
                    const uint8_t* ptr, uint8_t threshold);

                   
// Clear the display and frame buffer, reset cursor to (0,0)
void SSD1306_Clear(void);

// Set character cursor position
void SSD1306_SetCursor(uint8_t x, uint8_t y);

// Print a null-terminated ASCII string at cursor position
void SSD1306_OutString(const char* str);

// Print a single character at cursor position, advance cursor
void SSD1306_OutChar(char c);

// Print a 16-bit unsigned decimal number (right-justified 5 digits)
void SSD1306_OutUDec(uint16_t n);
                    

#endif