// main.c
// Lab 11 UART — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX (UART0 on PA0/PA1)
// Ported to: STM32 F446RE Nucleo (USART2 on PA2/PA3 via ST-LINK VCP)
//
// Logic is identical to the original — only UART_Init() changed hardware.
// All string conversion functions are unchanged from the original UART.c.
//
// HOW TO SEE OUTPUT:
//   Open a serial terminal on the ST-LINK COM port:
//     Baud: 115200, Data: 8, Parity: None, Stop: 1, Flow: None
//   Tools: PuTTY, TeraTerm, minicom, screen /dev/ttyACM0 115200
//   The port appears as:
//     Windows: COMx (check Device Manager → Ports)
//     Linux/Mac: /dev/ttyACM0 or /dev/tty.usbmodem...

#include "UART.h"

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