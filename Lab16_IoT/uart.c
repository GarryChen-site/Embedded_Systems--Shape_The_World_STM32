// uart.c — Lab 16 IoT Bare-Metal Dual UART Driver
//
// Direct register implementation without HAL
// Hardware:
//   - USART2: PA2(TX), PA3(RX) -> ST-LINK VCP @ 115200 baud
//   - USART1: PA9(TX), PA10(RX) -> NodeMCU ESP32 Link @ 115200 baud

#include "uart.h"
#include <stdint.h>

// =========================================================================
// RCC REGISTERS
// =========================================================================
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U)) // GPIO clocks
#define RCC_APB1ENR                                                            \
  (*((volatile uint32_t *)0x40023840U)) // USART2 clock (bit 17)
#define RCC_APB2ENR                                                            \
  (*((volatile uint32_t *)0x40023844U)) // USART1 clock (bit 4)

// =========================================================================
// GPIOA REGISTERS
// =========================================================================
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_OTYPER (*((volatile uint32_t *)0x40020004U))
#define GPIOA_OSPEEDR (*((volatile uint32_t *)0x40020008U))
#define GPIOA_PUPDR (*((volatile uint32_t *)0x4002000CU))
#define GPIOA_AFRL (*((volatile uint32_t *)0x40020020U)) // PA0..PA7
#define GPIOA_AFRH (*((volatile uint32_t *)0x40020024U)) // PA8..PA15

// =========================================================================
// USART1 REGISTERS
// =========================================================================
#define USART1_SR (*((volatile uint32_t *)0x40011000U))
#define USART1_DR (*((volatile uint32_t *)0x40011004U))
#define USART1_BRR (*((volatile uint32_t *)0x40011008U))
#define USART1_CR1 (*((volatile uint32_t *)0x4001100CU))

// =========================================================================
// USART2 REGISTERS
// =========================================================================
#define USART2_SR (*((volatile uint32_t *)0x40004400U))
#define USART2_DR (*((volatile uint32_t *)0x40004404U))
#define USART2_BRR (*((volatile uint32_t *)0x40004408U))
#define USART2_CR1 (*((volatile uint32_t *)0x4000440CU))

// Status register flag bits
#define USART_SR_TXE (1U << 7)  // Transmit data register empty
#define USART_SR_TC (1U << 6)   // Transmission complete
#define USART_SR_RXNE (1U << 5) // Read data register not empty
#define USART_SR_ORE (1U << 3)  // Overrun error

// Control register bits
#define USART_CR1_UE (1U << 13) // USART enable
#define USART_CR1_TE (1U << 3)  // Transmitter enable
#define USART_CR1_RE (1U << 2)  // Receiver enable

// -------------------------------------------------------------------------
// Baud rate: APB clock = 16 MHz (HSI)
// Target: 115200 baud -> USARTDIV = 16000000 / (16 * 115200) = 8.68055
// Mantissa = 8, Fraction = round(0.68055 * 16) = 11 (0xB)
// BRR = (8 << 4) | 0xB = 0x8B
// -------------------------------------------------------------------------
#define BAUD_115200_16MHZ 0x008BU

// =========================================================================
// SysTick Utility for Delays (1 ms resolution at 16 MHz)
// =========================================================================
#define STK_CTRL (*((volatile uint32_t *)0xE000E010U))
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U))
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))

void DelayMs(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i++) {
    // 16000 cycles at 16 MHz = 1 ms
    STK_LOAD = 16000U - 1U;
    STK_VAL = 0;
    STK_CTRL = 0x05; // Clock source = processor clock (1), counter enable (1)
    while (!(STK_CTRL & (1U << 16))) {
      // Wait for COUNTFLAG
    }
  }
  STK_CTRL = 0; // Disable
}

// =========================================================================
// USART2: PC Debug Console (PA2: TX, PA3: RX)
// =========================================================================
void UART2_Init(void) {
  // 1. Enable GPIOA and USART2 clocks
  RCC_AHB1ENR |= (1U << 0);  // GPIOAEN
  RCC_APB1ENR |= (1U << 17); // USART2EN

  // 2. Configure PA2, PA3 as Alternate Function AF7 (USART2)
  // MODER: 0b10 for bits [5:4] (PA2) and bits [7:6] (PA3)
  GPIOA_MODER &= ~((3U << 4) | (3U << 6));
  GPIOA_MODER |= ((2U << 4) | (2U << 6));

  // High speed
  GPIOA_OSPEEDR |= ((3U << 4) | (3U << 6));

  // Pull-up on PA3 (RX)
  GPIOA_PUPDR &= ~((3U << 4) | (3U << 6));
  GPIOA_PUPDR |= (1U << 6); // Pull-up on PA3

  // AFRL: AF7 (0b0111) for PA2 (bits 11:8) and PA3 (bits 15:12)
  GPIOA_AFRL &= ~((0xFU << 8) | (0xFU << 12));
  GPIOA_AFRL |= ((7U << 8) | (7U << 12));

  // 3. Configure Baud Rate: 115200 @ 16 MHz
  USART2_BRR = BAUD_115200_16MHZ;

  // 4. Enable Transmitter, Receiver, and USART module (8N1 default)
  USART2_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void UART2_OutChar(char c) {
  while (!(USART2_SR & USART_SR_TXE)) {
    // Wait until transmit buffer is empty
  }
  USART2_DR = (uint32_t)c;
}

void UART2_OutString(const char *str) {
  if (!str)
    return;
  while (*str) {
    if (*str == '\n') {
      UART2_OutChar('\r'); // Carriage return before newline
    }
    UART2_OutChar(*str);
    str++;
  }
}

char UART2_InChar(void) {
  while (!(USART2_SR & USART_SR_RXNE)) {
    // Wait until byte received
  }
  return (char)(USART2_DR & 0xFF);
}

char UART2_InCharNonBlocking(void) {
  if (USART2_SR & USART_SR_RXNE) {
    return (char)(USART2_DR & 0xFF);
  }
  return 0;
}

void UART2_OutUDec(uint32_t n) {
  if (n >= 10) {
    UART2_OutUDec(n / 10);
    n = n % 10;
  }
  UART2_OutChar((char)(n + '0'));
}

// =========================================================================
// USART1: ESP32 Link (PA9: TX, PA10: RX)
// =========================================================================
void UART1_Init(void) {
  // 1. Enable GPIOA and USART1 clocks
  RCC_AHB1ENR |= (1U << 0); // GPIOAEN
  RCC_APB2ENR |= (1U << 4); // USART1EN

  // 2. Configure PA9, PA10 as Alternate Function AF7 (USART1)
  // MODER: bits [19:18] for PA9, bits [21:20] for PA10
  GPIOA_MODER &= ~((3U << 18) | (3U << 20));
  GPIOA_MODER |= ((2U << 18) | (2U << 20));

  // High speed
  GPIOA_OSPEEDR |= ((3U << 18) | (3U << 20));

  // Pull-up on PA10 (RX)
  GPIOA_PUPDR &= ~((3U << 18) | (3U << 20));
  GPIOA_PUPDR |= (1U << 20); // Pull-up on PA10

  // AFRH: AF7 for PA9 (bits 7:4) and PA10 (bits 11:8)
  GPIOA_AFRH &= ~((0xFU << 4) | (0xFU << 8));
  GPIOA_AFRH |= ((7U << 4) | (7U << 8));

  // 3. Configure Baud Rate: 115200 @ 16 MHz
  USART1_BRR = BAUD_115200_16MHZ;

  // 4. Enable Transmitter, Receiver, and USART module (8N1 default)
  USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

  // Clear any pending overrun or stale read
  UART1_FlushRx();
}

void UART1_OutChar(char c) {
  while (!(USART1_SR & USART_SR_TXE)) {
    // Wait until transmit buffer is empty
  }
  USART1_DR = (uint32_t)c;
}

void UART1_OutString(const char *str) {
  if (!str) {
    return;
  }
  while (*str) {
    UART1_OutChar(*str);
    str++;
  }
}

char UART1_InCharNonBlocking(void) {
  if (USART1_SR & USART_SR_ORE) {
    // Overrun flag cleared by reading SR then DR
    volatile uint32_t dummy = USART1_SR;
    dummy = USART1_DR;
    (void)dummy;
  }
  if (USART1_SR & USART_SR_RXNE) {
    return (char)(USART1_DR & 0xFF);
  }
  return 0;
}

void UART1_FlushRx(void) {
  volatile uint32_t dummy = USART1_SR;
  dummy = USART1_DR;
  (void)dummy;
  while (USART1_SR & USART_SR_RXNE) {
    dummy = USART1_DR;
    (void)dummy;
  }
}

int UART1_ReadLine(char *buf, uint16_t maxLen, uint32_t timeoutMs) {
  if (!buf || maxLen < 2)
    return 0;
  uint16_t idx = 0;
  uint32_t elapsed = 0;

  while (elapsed < timeoutMs) {
    char c = UART1_InCharNonBlocking();
    if (c != 0) {
      if (c == '\r') {
        continue; // Ignore CR
      }
      if (c == '\n') {
        if (idx > 0) {
          buf[idx] = '\0';
          return 1; // Successfully got line
        }
        continue; // Skip leading empty lines
      }
      if (idx < maxLen - 1) {
        buf[idx++] = c;
      }
    } else {
      DelayMs(1);
      elapsed++;
    }
  }

  if (idx > 0) {
    buf[idx] = '\0';
    return 1;
  }
  buf[0] = '\0';
  return 0; // Timeout
}
