
#include "uart.h"

#include <stdint.h>

unsigned char String[10];

void UART_OutString(unsigned char *buffer) {
  uint32_t i = 0;
  while (buffer[i]) {
    UART_OutChar(buffer[i]);
    i++;
  }
}

unsigned long UART_InUDec(void) {
  unsigned long number = 0, length = 0;
  char character;
  character = UART_InChar();
  while (character != CR) {
    if ((character >= '0') && (character <= '9')) {
      number = 10 * number + (character - '0');
      length++;
      UART_OutChar(character);
    } else if ((character == BS) && length) {
      number /= 10;
      length--;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  return number;
}

void UART_ConvertUDec(unsigned long n) {
  int i;
  if (n > 9999) {
    for (i = 0; i < 4; i++) {
      String[i] = '*';
    }
  } else {
    for (i = 3; i >= 0; i--) {
      if (n > 0 || i == 3) {
        String[i] = (unsigned char)(n % 10 + 0x30);
      } else {
        String[i] = ' ';
      }
      n /= 10;
    }
  }
  String[4] = ' ';
  for (i = 5; i < 10; i++) {
    String[i] = 0;
  }
}

void UART_OutUDec(unsigned long n) {
  UART_ConvertUDec(n);
  UART_OutString(String);
}

void UART_ConvertDistance(unsigned long n) {
  int i;
  if (n > 9999) {
    for (i = 0; i < 5; i++) {
      String[i] = '*';
    }
  } else {
    for (i = 4; i > 0; i--) {
      String[i] = (unsigned char)(n % 10 + 0x30);
      n /= 10;
    }
  }

  String[0] = String[1];
  String[1] = '.';
  String[5] = ' ';
  String[6] = 'c';
  String[7] = 'm';
  String[8] = 0;
  String[9] = 0;
}

void UART_OutDistance(unsigned long n) {
  UART_ConvertDistance(n);
  UART_OutString(String);
}