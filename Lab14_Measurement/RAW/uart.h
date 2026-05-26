
#ifndef UART_H
#define UART_H
#include <stdint.h>


void UART2_Init(void);


void UART2_OutString(const char *str);


void UART2_OutChar(char c);

#endif