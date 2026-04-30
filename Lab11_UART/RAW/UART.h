#ifndef UART_H
#define UART_H

#include <stdint.h>

#define CR   0x0D   // carriage return '\r'
#define LF   0x0A   // line feed '\n'
#define BS   0x08   // backspace

//------------UART_Init------------
// Initialize USART2 for 115200 baud, 8N1
// PA2 = TX, PA3 = RX (connected to ST-LINK VCP)
// Input:  none
// Output: none
void UART_Init(void);


//------------UART_InChar------------
// Blocking read: wait for one character from the terminal
// Input:  none
// Output: received ASCII character
unsigned char UART_InChar(void);


//------------UART_InCharNonBlocking------------
// Non-blocking read: return 0 immediately if no data
// Input:  none
// Output: received ASCII character, or 0 if none available
unsigned char UART_InCharNonBlocking(void);


//------------UART_OutChar------------
// Blocking write: send one character to the terminal
// Input:  data — ASCII character to send
// Output: none
void UART_OutChar(unsigned char data);


//------------UART_OutString------------
// Send a null-terminated string to the terminal
// Input:  pointer to null-terminated string
// Output: none
void UART_OutString(unsigned char* buffer);


//------------UART_InUDec------------
// Read an unsigned decimal number typed by the user
// Echoes digits, supports backspace, terminates on Enter
// Input:  none
// Output: 32-bit unsigned number
unsigned long UART_InUDec(void);


//-----------------------UART_ConvertUDec-----------------------
// Convert a 32-bit number to 4-digit decimal string in String[]
// Fixed format: 4 digits + 1 space + null
// Examples:
//    4 → "   4 "
//   31 → "  31 "
//  102 → " 102 "
// 2210 → "2210 "
//10000 → "**** "
void UART_ConvertUDec(unsigned long n);


//-----------------------UART_OutUDec-----------------------
// Output 32-bit number in 4-digit decimal fixed format
// Input:  32-bit unsigned number
// Output: none
void UART_OutUDec(unsigned long n);


//-----------------------UART_ConvertDistance-----------------------
// Convert a 32-bit distance (0.001 cm resolution) to string in String[]
// Fixed format: 1 digit, '.', 3 digits, space, "cm", null
// Examples:
//    4 → "0.004 cm"
//   31 → "0.031 cm"
//  102 → "0.102 cm"
// 2210 → "2.210 cm"
//10000 → "*.*** cm"
void UART_ConvertDistance(unsigned long n);


//-----------------------UART_OutDistance-----------------------
// Output 32-bit distance in fixed-point format
// Input:  32-bit number (0.001 cm resolution)
// Output: none
void UART_OutDistance(unsigned long n);



#endif