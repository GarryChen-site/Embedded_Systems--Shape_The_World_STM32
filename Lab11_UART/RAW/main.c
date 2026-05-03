
#include "UART.h"
#include "uart.h"

int main(void) {

  unsigned long n;

  UART_Init();

  UART_OutString((unsigned char *)"Runing Lab 11\r\n");

  while (1) {
    UART_OutString((unsigned char *)"\n\rInput: ");
    n = UART_InUDec();

    n = UART_InUDec(); // read number from terminal

    UART_OutString((unsigned char *)" UART_OutUDec = ");
    UART_OutUDec(n); // "  31 " format

    UART_OutString((unsigned char *)",  UART_OutDistance = ");
    UART_OutDistance(n); // "0.031 cm" format
  }
}