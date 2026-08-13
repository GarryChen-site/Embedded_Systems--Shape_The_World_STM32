// uart.c
// Lab 5 FunctionsInC — STM32 F446RE Nucleo Port
//
// Bare-metal USART2 driver. Replaces the TM4C UART.c from the original lab.
// Same external interface (UART_Init, UART_InChar, UART_OutChar).
//
// -------------------------------------------------------------------------
// HARDWARE CONNECTION (how printf reaches your terminal):
//
//   STM32F446RE                 Nucleo board              Your PC
//   ┌──────────────┐           ┌────────────┐
//   │ USART2 TX    │──PA2──────│ ST-LINK    │──USB──▶ COM port / ttyUSB
//   │ USART2 RX    │──PA3──────│ VCP bridge │──USB──◀ terminal (115200)
//   └──────────────┘           └────────────┘
//
//   Source: UM1724 → "Hardware layout" → "USART communication"
//   PA2/PA3 are reserved pins — but here we are using them intentionally
//   for UART, which is exactly their purpose.
//
// -------------------------------------------------------------------------
// KEY DIFFERENCE FROM TM4C (UART.c):
//
//   TM4C used UART0 on PA0/PA1, clock 80 MHz:
//     UART0_IBRD_R = 43;   UART0_FBRD_R = 26;
//
//   STM32 uses USART2 on PA2/PA3 (AF7), clock 16 MHz HSI:
//     USART2_BRR = 0x8B;   (see calculation below)
//
//   TM4C had separate RCGC1 (UART clock) and RCGC2 (GPIO clock).
//   STM32 has APB1ENR (USART2 clock) and AHB1ENR (GPIOA clock).
//
//   TM4C used PCTL register to assign alternate function.
//   STM32 uses MODER=10 (AF) + AFR register to select which AF (AF7).
//
// -------------------------------------------------------------------------

#include "uart.h"
#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define RCC_APB1ENR (*((volatile uint32_t *)0x40023840U))

#define GPIOAEN (1U << 0)
#define UART2EN (1U << 17)

#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_OSPEEDR (*((volatile uint32_t *)0x40020008U))
#define GPIOA_AFRL (*((volatile uint32_t *)0x40020020U))

#define PA2_MODER_MASK (3U << 4)
#define PA2_MODER_AF (2U << 4)
#define PA3_MODER_MASK (3U << 6)
#define PA3_MODER_AF (2U << 6)

#define PA2_SPEED_MASK (3U << 4)
#define PA2_SPEED_HIGH (3U << 4)
#define PA3_SPEED_MASK (3U << 6)
#define PA3_SPEED_HIGH (3U << 6)

#define PA2_AFRL_MASK (0xFU << 8)
#define PA2_AF7 (7U << 8)
#define PA3_AFRL_MASK (0xFU << 12)
#define PA3_AF7 (7U << 12)

#define USART2_SR (*((volatile uint32_t *)0x40004400U))
#define USART2_DR (*((volatile uint32_t *)0x40004404U))
#define USART2_BRR (*((volatile uint32_t *)0x40004408U))
#define USART2_CR1 (*((volatile uint32_t *)0x4000440CU))

#define USART2_TXE (1U << 7)
#define USART2_RXNE (1U << 5)

#define USART_CR1_UE (1U << 13)
#define USART_CR1_TE (1U << 3)
#define USART_CR1_RE (1U << 2)

void UART_Init(void) {
  // 1. Enable GPIOA and UART2 clocks
  RCC_AHB1ENR |= GPIOAEN;
  RCC_APB1ENR |= UART2EN;

  // 2. Configure PA2 and PA3 as alternate function (AF7)
  GPIOA_MODER &= ~(PA2_MODER_MASK | PA3_MODER_MASK);
  GPIOA_MODER |= (PA2_MODER_AF | PA3_MODER_AF);

  GPIOA_OSPEEDR &= ~(PA2_SPEED_MASK | PA3_SPEED_MASK);
  GPIOA_OSPEEDR |= (PA2_SPEED_HIGH | PA3_SPEED_HIGH);

  GPIOA_AFRL &= ~(PA2_AFRL_MASK | PA3_AFRL_MASK);
  GPIOA_AFRL |= (PA2_AF7 | PA3_AF7);

  // 3. Disable UART2 before configuration
  USART2_CR1 &= ~USART_CR1_UE;

  // 4. Set baud rate
  // STM32 (16 MHz HSI, APB1 = 16 MHz):
  //   OVER8 = 0 (16× oversampling, default)  RM0390 §27.3.4
  //   USARTDIV = fPCLK1 / (16 × baud)
  //            = 16,000,000 / (16 × 115,200)
  //            = 8.6806...
  //   DIV_Mantissa = 8    (integer part)
  //   DIV_Fraction = round(0.6806 × 16) = round(10.89) = 11 = 0xB
  //   BRR = (8 << 4) | 0xB = 0x8B = 139
  USART2_BRR = 0x8B;

  // 5. Enable UART2 (enable TX, RX and UART)
  USART2_CR1 |= (USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);
}

unsigned char UART_InChar(void) {
  while (!(USART2_SR & USART2_RXNE))
    ;
  return (unsigned char)USART2_DR;
}

void UART_OutChar(unsigned char data) {
  while (!(USART2_SR & USART2_TXE))
    ;
  USART2_DR = data;
}