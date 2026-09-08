// uart.c — Lab 14 ADC, STM32 F446RE port
//
// USART2 at 9600 baud, 8N1 — replaces Nokia5110 LCD.
// PA2 = TX → goes via ST-Link to USB virtual COM port on PC.
//
// ORIGINAL LAB14 (TM4C):
//   Nokia5110_Init(), Nokia5110_Clear(), Nokia5110_OutString()
//
// THIS PORT (STM32):
//   UART2_Init(), UART2_OutString()
//   Output visible on PC terminal (PuTTY, minicom, etc.) at 9600 8N1.
//
// BAUD RATE CALCULATION:
//   USARTDIV = fPCLK1 / (16 × BaudRate) = 16,000,000 / (16 × 9600)
//            = 104.1667
//   BRR mantissa = 104 = 0x68
//   BRR fraction = round(0.1667 × 16) = round(2.667) = 3
//   BRR = (104 << 4) | 3 = 0x0683
//   Actual baud = 16,000,000 / (16 × 104.1875) ≈ 9597 baud (error < 0.04%)

#include "uart.h"
#include <stdint.h>

// =========================================================================
// RCC REGISTERS
// =========================================================================

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define RCC_APB1ENR (*((volatile uint32_t *)0x40023840U))

#define GPIOAEN (1U << 0)
#define UART2EN (1U << 17)

// =========================================================================
// GPIOA REGISTERS
// =========================================================================
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_AFRL (*((volatile uint32_t *)0x40020020U))

// =========================================================================
// UART2 REGISTERS
// =========================================================================
#define USART2_SR (*((volatile uint32_t *)0x40004400U))
#define USART2_DR (*((volatile uint32_t *)0x40004404U))
#define USART2_BRR (*((volatile uint32_t *)0x40004408U))
#define USART2_CR1 (*((volatile uint32_t *)0x4000440CU))

// SR bits
#define USART_SR_TC (1U << 6)  // Transmit complete
#define USART_SR_TXE (1U << 7) // Transmit data register empty

// CR1 bits
#define USART_CR1_TE (1U << 3)  // Transmitter enable
#define USART_CR1_UE (1U << 13) // USART enable

void UART2_Init(void) {

  // 1. Enable GPIOA clocks
  RCC_AHB1ENR |= GPIOAEN;
  volatile uint32_t delay = RCC_AHB1ENR;
  (void)delay;

  // 2. Enable USART2 clock
  RCC_APB1ENR |= UART2EN;
  delay = RCC_AHB1ENR;
  (void)delay;

  // 3. PA2 → alternate function mode (MODER[5:4] = 10)
  GPIOA_MODER &= ~(3U << 4);
  GPIOA_MODER |= (2U << 4);

  // 4. PA2 → AF7 (USART2)  (AFRL bits [11:8] = 0111)
  GPIOA_AFRL &= ~(0xFU << 8);
  GPIOA_AFRL |= (0x7U << 8);

  // 1).  USARTDIV  = fPCLK / (16 × Baud)        → e.g. 104.1667
  // 2).  Mantissa  = floor(USARTDIV)             → 104 = 0x68
  // 3).  Fraction  = round(frac × 16)           → round(0.1667×16) = 3
  // 4).  BRR       = (Mantissa << 4) | Fraction → 0x0683
  USART2_BRR = 0x0683U;

  // 6. Enable USART2 and its transmitter
  USART2_CR1 = USART_CR1_UE | USART_CR1_TE;
}

void UART2_OutChar(char c) {
  while (!(USART2_SR & USART_SR_TXE))
    ;
  USART2_DR = c;
}

void UART2_OutString(const char *str) {
  while (*str) {
    UART2_OutChar(*str);
    str++;
  }
}