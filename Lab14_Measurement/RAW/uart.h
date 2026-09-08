// uart.h — Lab 14 ADC, STM32 F446RE port
//
// Minimal UART2 driver — replaces Nokia5110 LCD display.
// USART2 is connected to the ST-Link virtual COM port (USB serial).
// On PC: open any terminal at 9600 8N1 on the ST-Link COM port.
//
// Pin mapping (fixed by Nucleo hardware — UM1724):
//   PA2  USART2_TX  (AF7)  → ST-Link → USB → PC terminal
//   PA3  USART2_RX  (AF7)  ← ST-Link ← USB ← PC terminal (not used here)
#ifndef UART_H
#define UART_H
#include <stdint.h>

// UART2_Init — configure USART2 at 9600 baud, 8N1
void UART2_Init(void);

// UART2_OutString — transmit null-terminated string
void UART2_OutString(const char *str);

// UART2_OutChar — transmit one byte
void UART2_OutChar(char c);

#endif