// i2c.h — Lab 14 ADC, STM32 F446RE
//
// Bare-metal I2C1 driver (polling, no DMA, no interrupts).
// Target: 400 kHz fast mode on PB6 (SCL) / PB7 (SDA) — default CubeIDE pins.
//
// HARDWARE CONNECTIONS (SSD1306 0.96" OLED):
//   OLED GND  →  Nucleo GND
//   OLED VCC  →  Nucleo 3.3V  (CN6 pin 4)
//   OLED SCL  →  PB6  (CN10 pin 17)
//   OLED SDA  →  PB7  (CN10 pin 19)
//
// NOTE: The OLED module PCB includes on-board pull-up resistors.
//       No external pull-ups are needed on the breadboard.
//
#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// =========================================================================
// I2C1_Init — configure I2C1 at 400 kHz (fast mode) on PB6 (SCL) / PB7 (SDA)// Call once before any I2C transfer.
// =========================================================================
void I2C1_Init(void);


// =========================================================================
// I2C1_Send — write 'len' bytes from 'buf' to 7-bit slave 'addr7'.
//
// Sends:  START → (addr7 << 1 | 0) → buf[0] → buf[1] → … → STOP
// =========================================================================
void I2C1_Send(uint8_t addr7, const uint8_t *buf, uint32_t len);

// =========================================================================
// I2C1_SendFrag — write a prefix byte followed by 'len' bytes of 'data'
//                 in a single I2C transaction (no STOP between prefix and data).
//
// Sends:  START → (addr7 << 1 | 0) → pre → data[0] → … → data[len-1] → STOP
//
// Use case: SSD1306 needs a control byte (0x00 or 0x40) followed by the
// payload (commands or GDDRAM data) in one uninterrupted transfer.
// This avoids allocating a (1+1024)-byte stack buffer.
// =========================================================================
void I2C1_SendFrag(uint8_t addr7, uint8_t pre, const uint8_t* data, uint32_t len);


#endif
