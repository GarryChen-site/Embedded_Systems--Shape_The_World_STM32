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
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define RCC_APB1ENR (*((volatile uint32_t *)0x40023840U))

#define GPIOAEN (1U << 0)
#define UART2EN (1U << 17)

// =========================================================================
// GPIOA REGISTERS
// =========================================================================
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_AFRL (*((volatile uint32_t *)0x40020020U))

// PA2 MODER[5:4] = 10 (alternate function)
// PA3 MODER[7:6] = 10 (alternate function)
#define PA23_MODER_MASK (0xFU << 4U)
#define PA23_MODER_AF (0xAU << 4U)

// PA2 AFRL[11:8] = 7 (AF7 = USART2_TX)
// PA3 AFRL[15:12] = 7 (AF7 = USART2_RX)
#define PA23_AFRL_MASK (0xFFU << 8U)
#define PA23_AFRL_AF7 (0x77U << 8U) // 0111 0111

// =========================================================================
// UART2 REGISTERS
// =========================================================================
#define USART2_SR (*((volatile uint32_t *)0x40004400U))
#define USART2_DR (*((volatile uint32_t *)0x40004404U))
#define USART2_BRR (*((volatile uint32_t *)0x40004408U))
#define USART2_CR1 (*((volatile uint32_t *)0x4000440CU))

// SR bits
#define USART_SR_RXNE (1U << 5) // Receive not empty
#define USART_SR_TXE (1U << 7)  // Transmit data register empty

// CR1 bits
#define USART_CR1_RE (1U << 2)  // Receiver enable
#define USART_CR1_TE (1U << 3)  // Transmitter enable
#define USART_CR1_UE (1U << 13) // USART enable

unsigned char String[10];

// =========================================================================
// UART_Init
// Configure USART2 at 115200 baud, 8N1 on PA2(TX) / PA3(RX).
//
// TM4C original steps         → STM32 equivalent
// ─────────────────────────   → ─────────────────────────────────────────
// SYSCTL_RCGC1 |= UART0       → RCC_APB1ENR |= USART2EN
// SYSCTL_RCGC2 |= GPIOA       → RCC_AHB1ENR |= GPIOAEN
// UART0_CTL &= ~UARTEN         → USART2_CR1 &= ~UE  (disable first)
// UART0_IBRD = 43              → USART2_BRR = 0x8B  (single register)
// UART0_FBRD = 26              → (fraction embedded in BRR[3:0])
// UART0_LCRH = WLEN_8|FEN     → CR1 default = 8N1 (reset state)
// UART0_CTL |= UARTEN          → USART2_CR1 |= UE|TE|RE
// GPIO_PORTA_AFSEL |= 0x03    → GPIOA_MODER bits = AF
// GPIO_PORTA_PCTL = 0x11      → GPIOA_AFRL = AF7 for PA2, PA3
// =========================================================================
void UART_Init(void) {
  // 1. Enable clocks
  RCC_AHB1ENR |= GPIOAEN;
  RCC_APB1ENR |= UART2EN;
  volatile uint32_t dummy = RCC_APB1ENR;
  (void)dummy;

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

// =========================================================================
// UART_InChar
// Blocking receive: wait until RXNE=1, then read one byte.
//
// TM4C: while((UART0_FR_R & UART_FR_RXFE) != 0);  wait while EMPTY
// STM32: while(!(USART2_SR & RXNE));               wait while NOT FULL
// Semantically identical — both spin until a byte is available.
// =========================================================================
unsigned char UART_InChar(void) {
  while (!(USART2_SR & USART_SR_RXNE)) {
  };
  return (unsigned char)(USART2_DR & 0xFF);
}

// =========================================================================
// UART_InCharNonBlocking
// Return received byte immediately, or 0 if none available.
// =========================================================================
unsigned char UART_InCharNonBlocking(void) {
  if (USART2_SR & USART_SR_RXNE) {
    return (unsigned char)(USART2_DR & 0xFFU);
  }
  return 0;
}

// =========================================================================
// UART_OutChar
// Blocking transmit: wait until TXE=1, then write one byte.
//
// TM4C: while((UART0_FR_R & UART_FR_TXFF) != 0);  wait while FULL
// STM32: while(!(USART2_SR & TXE));                wait while NOT EMPTY
// =========================================================================
void UART_OutChar(unsigned char data) {
  while (!(USART2_SR & USART_SR_TXE)) {
  }; // wait until TX register empty
  USART2_DR = data;
}

// =========================================================================
// UART_OutString — identical logic to original, no hardware dependency
// =========================================================================
void UART_OutString(unsigned char *buffer) {
  uint32_t i = 0;
  while (buffer[i]) {
    UART_OutChar(buffer[i]);
    i++;
  }
}

// =========================================================================
// UART_InUDec — identical to original (hardware-independent logic)
// Reads digits '0'-'9', echoes them, supports backspace, Enter terminates.
// =========================================================================
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

// =========================================================================
// UART_ConvertUDec — identical to original (pure software, no hardware)
// Fills String[10] with a 4-digit right-justified decimal + space + null.
// =========================================================================
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

// =========================================================================
// UART_OutUDec — identical to original
// =========================================================================
void UART_OutUDec(unsigned long n) {
  UART_ConvertUDec(n);
  UART_OutString(String);
}

// =========================================================================
// UART_ConvertDistance — identical to original (pure software)
// Fills String[10] with "X.XXX cm\0" for n in 0.001 cm units.
// =========================================================================
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

// =========================================================================
// UART_OutDistance — identical to original
// =========================================================================
void UART_OutDistance(unsigned long n) {
  UART_ConvertDistance(n);
  UART_OutString(String);
}