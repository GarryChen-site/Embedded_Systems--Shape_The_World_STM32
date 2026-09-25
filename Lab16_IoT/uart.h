// uart.h — Lab 16 IoT Bare-Metal Dual UART Driver
//
// Dual UART configuration:
//   1. USART2: PA2 (TX) / PA3 (RX) — ST-LINK Virtual COM Port to PC (115200 baud, 8N1)
//   2. USART1: PA9 (TX) / PA10 (RX) — Communication link to NodeMCU ESP32 (115200 baud, 8N1)
//

#ifndef UART_H
#define UART_H

#include <stdint.h>

// -----------------------------------------------------------------------------
// PC Console UART (USART2: PA2/PA3)
// -----------------------------------------------------------------------------
void UART2_Init(void);
void UART2_OutChar(char c);
void UART2_OutString(const char *str);
char UART2_InChar(void);
char UART2_InCharNonBlocking(void);
void UART2_OutUDec(uint32_t n);

// -----------------------------------------------------------------------------
// ESP32 Coprocessor Link (USART1: PA9/PA10)
// -----------------------------------------------------------------------------
void UART1_Init(void);
void UART1_OutChar(char c);
void UART1_OutString(const char *str);
char UART1_InCharNonBlocking(void);
void UART1_FlushRx(void);

// Read a single line terminated by '\n' into buf (null-terminated, '\r' and '\n' stripped)
// Returns 1 if a line was received, 0 on timeout.
int UART1_ReadLine(char *buf, uint16_t maxLen, uint32_t timeoutMs);

// Millisecond delay utility (SysTick or calibrated loop)
void DelayMs(uint32_t ms);

#endif // UART_H
