// main.c
// Lab 5 FunctionsInC — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX
// Ported to: STM32 F446RE Nucleo
//
// -------------------------------------------------------------------------
// WHAT THIS LAB TEACHES:
//   Writing and calling C functions (Calc_Area).
//   The application logic is IDENTICAL to the original.
//   Only the UART driver underneath changes.
//
// CHANGES FROM ORIGINAL:
//   - TExaS_Init()      removed (TI grader, not needed)
//   - EnableInterrupts() removed (grader only, not needed)
//   - UART_Init()        stays — but now calls our STM32 UART driver
//   - printf / scanf     stay — redirected to USART2 via newlib syscalls
//
// HOW printf WORKS ON STM32 (no OS):
//   printf() → newlib → _write() syscall → UART_OutChar() → USART2 → USB
//   scanf()  → newlib → _read()  syscall → UART_InChar()  ← USB ← terminal
//   See syscalls.c for the _write/_read implementation.
//   The USB connection is the Nucleo's ST-LINK Virtual COM Port (VCP).
//
// HARDWARE:
//   USART2 TX → PA2 (AF7) → ST-LINK → USB → your terminal
//   USART2 RX → PA3 (AF7) → ST-LINK → USB → your keyboard
//
// -------------------------------------------------------------------------
#include "uart.h"  // UART_Init(), UART_InChar(), UART_OutChar()
#include <stdio.h> // printf, scanf — redirected to USART2 via syscalls.c

// Function prototype
unsigned long Calc_Area(unsigned long l, unsigned long w);

int main(void) {

  UART_Init(); // initialize USART2 at 115200 baud, 8N1

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);

  unsigned long length, width, area;
  printf("\r\nThis program calculates areas of rectangular rooms.\r\n");

  while (1) {
    printf("\r\nGive length: ");
    scanf("%ld", &length);

    printf("\r\nGive width: ");
    scanf("%ld", &width);

    area = Calc_Area(length, width);
    printf("\r\nArea of the room = %ld\r\n", area);
  }
}

// =========================================================================
// Calc_Area
// Computes the area of a rectangular room.
// Input:  l = length, w = width (dimensions in feet or any unit)
// Output: l × w, or 0 if either dimension is outside [3, 20]
//
// This function is IDENTICAL to the original TM4C lab.
// =========================================================================
unsigned long Calc_Area(unsigned long l, unsigned long w) {
  if (l < 3 || l > 20 || w < 3 || w > 20) {
    return 0;
  }
  return l * w;
}
