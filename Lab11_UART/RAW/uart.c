// uart.c
// Lab 11 UART — STM32 F446RE Nucleo Port
//
// Original: TM4C123 UART0 on PA0(RX)/PA1(TX) via USB debug cable
// Ported to: STM32 USART2 on PA2(TX)/PA3(RX) via ST-LINK Virtual COM Port
//
// -------------------------------------------------------------------------
// KEY DIFFERENCE FROM TM4C:
//
//   TM4C UART0                     STM32 USART2
//   ────────────────────────────   ────────────────────────────────────
//   PA0 = U0Rx, PA1 = U0Tx         PA2 = USART2_TX, PA3 = USART2_RX
//   Clock: SYSCTL_RCGC1            Clock: RCC_APB1ENR (bit 17)
//   Baud: IBRD=43, FBRD=26         Baud: BRR=0x8B (see below)
//   TX wait: UART_FR_TXFF          TX wait: USART2_SR bit 7 (TXE)
//   RX wait: UART_FR_RXFE          RX wait: USART2_SR bit 5 (RXNE)
//   Data:   UART0_DR_R             Data:   USART2_DR
//
// -------------------------------------------------------------------------
// BAUD RATE CALCULATION:
//   Clock source: APB1 = HSI = 16 MHz (no PLL)
//   Target baud:  115200
//
//   USARTDIV = f_CK / (16 × baud) = 16,000,000 / (16 × 115,200) = 8.6805...
//   Mantissa (BRR[15:4]) = 8
//   Fraction (BRR[3:0])  = round(0.6805 × 16) = round(10.89) = 11
//   BRR = (8 << 4) | 11 = 0x8B
//
//   Actual baud = 16,000,000 / (16 × 8.6875) = 115,108 baud
//   Error = (115,200 - 115,108) / 115,200 = 0.08%  ✓ (limit: ±2%)
//
// -------------------------------------------------------------------------
// ALTERNATE FUNCTION MAPPING:
//   PA2: AF7 = USART2_TX   → GPIOA_AFRL bits [11:8]  = 0x7
//   PA3: AF7 = USART2_RX   → GPIOA_AFRL bits [15:12] = 0x7
//
// -------------------------------------------------------------------------
// HARDWARE PATH:
//   STM32 PA2(TX) → ST-LINK → USB → PC terminal (115200 baud)
//   PC terminal → USB → ST-LINK → STM32 PA3(RX)
//
// Open any serial terminal at 115200, 8N1:
//   - PuTTY, TeraTerm, minicom, screen /dev/ttyACM0 115200
//
// -------------------------------------------------------------------------
// REFERENCE MANUALS:
//   RM0390   — RCC APB1 peripheral clock enable
//   RM0390   — GPIO alternate function configuration
//   RM0390   — USART registers and operation
//   UM1724   — "USART communication" section (ST-LINK VCP pin mapping)
// -------------------------------------------------------------------------
#include "uart.h"
#include <stdint.h>

// =========================================================================
// RCC REGISTERS
// =========================================================================
#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define RCC_APB1ENR (*((volatile unsigned int *)0x40023840))

#define GPIOAEN (1 << 0)
#define UART2EN (1 << 17)

// =========================================================================
// GPIOA REGISTERS
// =========================================================================
#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_AFRL (*((volatile unsigned int *)0x40020020))

// PA2 MODER[5:4] = 10 (alternate function)
// PA3 MODER[7:6] = 10 (alternate function)
#define PA23_MODER_MASK (0xF << 4)
#define PA23_MODER_AF (0xA << 4)

// PA2 AFRL[11:8] = 7 (AF7 = USART2_TX)
// PA3 AFRL[15:12] = 7 (AF7 = USART2_RX)
#define PA23_AFRL_MASK (0xFFU << 8)
#define PA23_AFRL_AF7 (0x77U << 8) // 0111 0111

// =========================================================================
// UART2 REGISTERS
// =========================================================================
#define USART2_SR (*((volatile unsigned int *)0x40004400))
#define USART2_DR (*((volatile unsigned int *)0x40004404))
#define USART2_BRR (*((volatile unsigned int *)0x40004408))
#define USART2_CR1 (*((volatile unsigned int *)0x4000440C))

// SR bits
#define USART_SR_RXNE (1 << 5) // Receive not empty
#define USART_SR_TXE (1 << 7)  // Transmit data register empty

// CR1 bits
#define USART_CR1_RE (1 << 2)  // Receiver enable
#define USART_CR1_TE (1 << 3)  // Transmitter enable
#define USART_CR1_UE (1 << 13) // USART enable

unsigned char String[10];

void UART_Init(void) {
  // 1. Enable clocks
  RCC_AHB1ENR |= GPIOAEN;
  RCC_APB1ENR |= UART2EN;

  // 2. Configure PA2(TX) and PA3(RX) as Alternate Function
  GPIOA_MODER &= ~PA23_MODER_MASK;
  GPIOA_MODER |= PA23_MODER_AF; // 10 = alternate function

  GPIOA_AFRL &= ~PA23_AFRL_MASK;
  GPIOA_AFRL |= PA23_AFRL_AF7; // AF7 = UASRT2

  // 3. Configure UART2
  USART2_CR1 &= ~USART_CR1_UE; // Disable UART2 before configuration

  USART2_BRR = 0x8B; // 115200 baud

  USART2_CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

unsigned char UART_InChar(void) {
  while (!(USART2_SR & USART_SR_RXNE)) {
  };
  return (unsigned char)(USART2_DR & 0xFF);
}

unsigned char UART_InCharNonBlocking(void) {
  if (USART2_SR & USART_SR_RXNE) {
    return (unsigned char)(USART2_DR & 0xFFU);
  }
  return 0;
}

void UART_OutChar(unsigned char data) {
  while (!(USART2_SR & USART_SR_TXE)) {
  }; // wait until TX register empty
  USART2_DR = data;
}

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