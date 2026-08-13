// uart.h
// Lab 5 FunctionsInC — STM32 F446RE Nucleo Port
//
// Interface is identical to the original TM4C UART.h so that main.c
// does not need to change. Only the implementation (uart.c) differs.


#ifndef UART_H
#define UART_H


// Initialize USART2: 115200 baud, 8-bit, no parity, 1 stop bit, FIFOs
// PA2 = TX (AF7), PA3 = RX (AF7)
void UART_Init(void);


// Block until a character is received, then return it
unsigned char UART_InChar(void);


// Block until TX register is empty, then transimit one character
void UART_OutChar(unsigned char data);





#endif // UART_H