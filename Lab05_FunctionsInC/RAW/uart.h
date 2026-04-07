
#ifndef UART_H
#define UART_H


void UART_Init(void);


// Block until a character is received, then return it
unsigned char UART_InChar(void);


// Block until TX register is empty, then transimit one character
void UART_OutChar(unsigned char data);





#endif