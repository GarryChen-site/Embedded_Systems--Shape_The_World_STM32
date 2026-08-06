#include "esp32_at.h"
#include <string.h>
// ============================================================
// Register definitions
// ============================================================

// RCC
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U)) // §6.3.9
#define RCC_APB2ENR (*((volatile uint32_t *)0x40023844U)) // §6.3.15

// GPIOA
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_AFRH (*((volatile uint32_t *)0x40020024U)) // pins 8-15

// USART1
#define USART1_SR (*((volatile uint32_t *)0x40011000U))  // status
#define USART1_DR (*((volatile uint32_t *)0x40011004U))  // data
#define USART1_BRR (*((volatile uint32_t *)0x40011008U)) // baud
#define USART1_CR1 (*((volatile uint32_t *)0x4001100CU)) // control 1

// USART1 SR flag bits
#define USART_SR_TXE (1U << 7)  // transmit data reg empty
#define USART_SR_RXNE (1U << 5) // read data reg not empty

// USART1 CR1 control bits
#define USART_CR1_UE (1U << 13) // USART enable
#define USART_CR1_TE (1U << 3)  // transmitter enable
#define USART_CR1_RE (1U << 2)  // receiver enable

// ============================================================
// Internal helpers
// ============================================================

// Delay approximately N milliseconds
static void delay_ms(uint32_t ms) {
  while (ms > 0U) {
    volatile uint32_t i = 2667U; // ~1 ms delay at 16MHz HCLK
    while (i > 0U) {
      i--;
    }
    ms--;
  }
}

// Send one character on USART1(blocking)
static void usart1_putc(char c) {
  while (!(USART1_SR & USART_SR_TXE))
    ;
  USART1_DR = (uint32_t)(uint8_t)c;
}

// Send null-terminated string on USART1
static void usart1_puts(const char *s) {
  while (*s) {
    usart1_putc(*s++);
  }
}

// Spin-receive until 'token' appears in the incomming byte strea,
// or until timeout_ms milliseconds elapse;
// Returns 1 on match, 0 on timeout
static int wait_for(const char *token, uint32_t timeout_ms) {
  uint32_t tok_len = (uint32_t)strlen(token);
  uint32_t match = 0U;

  uint32_t total_loops = timeout_ms * 4000U;
  char ch;

  while (total_loops > 0U) {
    if (USART1_SR & USART_SR_RXNE) {
      ch = (char)(USART1_DR & 0xFFU);
      if (ch == token[match]) {
        match++;
        if (match == tok_len) {
          return 1;
        }
      } else {
        match = (ch == token[0]) ? 1U : 0U;
      }
    }
    total_loops--;
  }
  return 0;
}

// ============================================================
// Public API
// ============================================================

// ============================================================
// ESP32_Init - configure USART1 (PA9/PA10, 115200 8N1)
//
// BRR = round(PCLK2 / baud) = round(16,000,000 / 115,200) = 139 = 0x008B
// ============================================================
void ESP32_Init(void) {
  RCC_AHB1ENR |= (1U << 0);
  RCC_APB2ENR |= (1U << 4);

  GPIOA_MODER &= ~((3U << 18) | (3U << 20));
  GPIOA_MODER |= ((2U << 18) | (2U << 20));

  GPIOA_AFRH &= ~((0xFU << 4) | (0xFU << 8));
  GPIOA_AFRH |= ((7U << 4) | (7U << 8));

  USART1_CR1 = 0U;
  USART1_BRR = 0x008BU;
  USART1_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

  delay_ms(200U);
}

// ============================================================
// ESP32_ConnectWiFi - reset ESP32, set station mode, join AP
// ============================================================
int ESP32_ConnectWiFi(const char *ssid, const char *pw) {
  // Reset ESP32
  usart1_puts("AT+RST\r\n");
  if (!wait_for("ready", 5000U)) {
    return -1;
  }

  // Station mode
  usart1_puts("AT+CWMODE=1\r\n");
  if (!wait_for("OK", 2000U)) {
    return -2;
  }

  // Join AP
  usart1_puts("AT+CWJAP=\"");
  usart1_puts(ssid);
  usart1_puts("\",\"");
  usart1_puts(pw);
  usart1_puts("\"\r\n");
  if (!wait_for("WIFI GOT IP", 20000U)) {
    return -3;
  }

  return 0;
}