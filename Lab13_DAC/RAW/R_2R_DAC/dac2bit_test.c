// dac2bit_test.c — 2-bit R-2R DAC verification
// STM32F446RE Nucleo, 16 MHz HSI (no PLL needed)
//
// WIRING (in addition to the R-2R breadboard already built):
//   PB3  → breadboard D0 input (row 3, any column)
//   PB4  → breadboard D1 input (row 9, any column)
//   PA0  → breadboard VOUT     (row 11, any column)   [CN8 pin 1 = A0]
//   GND  → breadboard GND rail
//   3.3V → breadboard 3V3 rail
//   USB  → PC  (Nucleo virtual COM port, already connected)
//
// HOW TO SEE OUTPUT:
//   Windows: Device Manager → Ports → find "STMicroelectronics STLink Virtual
//   COM"
//            Open PuTTY / Tera Term at 9600 baud on that COMx
//   Mac/Linux: screen /dev/tty.usbmodem* 9600
//
// EXPECTED UART OUTPUT:
//   === 2-bit R-2R DAC Checker ===
//   D1  D0   Expected    Measured    Status
//   -----------------------------------------
//    0   0   0.00 V      0.01 V      OK
//    0   1   0.83 V      0.82 V      OK
//    1   0   1.65 V      1.64 V      OK
//    1   1   2.48 V      2.47 V      OK
//
// LED (LD2, PA5) blinks 1/2/3/4 times to show which step is active.
//
// Paste this as main.c in a new STM32CubeIDE project (STM32F446RE, no IOC
// peripheral config needed — everything is configured in bare-metal registers).
// ==========================================================================

#include <stdint.h>

// ── RCC ────────────────────────────────────────────────────────────────────
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830U)
#define RCC_APB1ENR (*(volatile uint32_t *)0x40023840U)
#define RCC_APB2ENR (*(volatile uint32_t *)0x40023844U)

// ── GPIOA ──────────────────────────────────────────────────────────────────
#define GPIOA_MODER (*(volatile uint32_t *)0x40020000U)
#define GPIOA_ODR (*(volatile uint32_t *)0x40020014U)
#define GPIOA_AFRL (*(volatile uint32_t *)0x40020020U)

// ── GPIOB ──────────────────────────────────────────────────────────────────
#define GPIOB_MODER (*(volatile uint32_t *)0x40020400U)
#define GPIOB_ODR (*(volatile uint32_t *)0x40020414U)

// ── USART2 (PA2=TX → ST-Link virtual COM) ──────────────────────────────────
#define USART2_SR (*(volatile uint32_t *)0x40004400U)
#define USART2_DR (*(volatile uint32_t *)0x40004404U)
#define USART2_BRR (*(volatile uint32_t *)0x40004408U)
#define USART2_CR1 (*(volatile uint32_t *)0x4000440CU)

// ── ADC1 (PA0 = channel 0) ─────────────────────────────────────────────────
#define ADC1_SR (*(volatile uint32_t *)0x40012000U)
#define ADC1_CR2 (*(volatile uint32_t *)0x40012008U)
#define ADC1_SMPR2 (*(volatile uint32_t *)0x40012014U)
#define ADC1_SQR3 (*(volatile uint32_t *)0x4001234CU)
#define ADC1_DR (*(volatile uint32_t *)0x4001204CU)
#define ADC_CCR (*(volatile uint32_t *)0x40012304U)

// ── SysTick ────────────────────────────────────────────────────────────────
#define STK_CTRL (*(volatile uint32_t *)0xE000E010U)
#define STK_LOAD (*(volatile uint32_t *)0xE000E014U)
#define STK_VAL (*(volatile uint32_t *)0xE000E018U)

// ═══════════════════════════════════════════════════════════════════════════
// Delay
// ═══════════════════════════════════════════════════════════════════════════
static void delay_ms(uint32_t ms) {
  STK_CTRL = 0;
  while (ms--) {
    STK_LOAD = 16000U - 1U; // 16 MHz → 1 ms
    STK_VAL = 0;
    STK_CTRL = 0x5U; // ENABLE | CLKSOURCE(AHB)
    while (!(STK_CTRL & (1U << 16)))
      ;
  }
  STK_CTRL = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// UART helpers
// ═══════════════════════════════════════════════════════════════════════════
static void uart_putc(char c) {
  while (!(USART2_SR & (1U << 7)))
    ;
  USART2_DR = (uint8_t)c;
}
static void uart_puts(const char *s) {
  while (*s)
    uart_putc(*s++);
}
static void uart_putu(uint32_t n) {
  char buf[10];
  int i = 0;
  if (!n) {
    uart_putc('0');
    return;
  }
  while (n) {
    buf[i++] = '0' + (char)(n % 10);
    n /= 10;
  }
  while (i > 0)
    uart_putc(buf[--i]);
}

// Print 12-bit ADC as "X.XX V" (Vref = 3.30 V)
static void uart_print_v(uint32_t adc) {
  uint32_t mv = (adc * 3300U) / 4095U;
  uart_putu(mv / 1000U);
  uart_putc('.');
  uint32_t f = mv % 1000U;
  uart_putc('0' + (char)(f / 100U));
  uart_putc('0' + (char)((f % 100U) / 10U));
  uart_puts(" V  ");
}

// Print OK / WARN (tolerance ±150 mV)
static void uart_check(uint32_t adc, uint32_t exp_mv) {
  int32_t mv = (int32_t)((adc * 3300U) / 4095U);
  int32_t diff = mv - (int32_t)exp_mv;
  if (diff < 0)
    diff = -diff;
  uart_puts(diff <= 150 ? "OK\r\n" : "WARN — check wiring\r\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// Peripheral init
// ═══════════════════════════════════════════════════════════════════════════
static void gpio_init(void) {
  RCC_AHB1ENR |= (1U << 0) | (1U << 1); // GPIOA + GPIOB
  volatile uint32_t d = RCC_AHB1ENR;
  (void)d;

  GPIOA_MODER |= (3U << 0); // PA0 = analog (ADC)

  // PA2 = AF7 (USART2 TX), PA3 = AF7 (USART2 RX)
  GPIOA_MODER &= ~((3U << 4) | (3U << 6));
  GPIOA_MODER |= ((2U << 4) | (2U << 6));
  GPIOA_AFRL &= ~0x0000FF00U;
  GPIOA_AFRL |= (7U << 8) | (7U << 12);

  // PA5 = output (LD2)
  GPIOA_MODER &= ~(3U << 10);
  GPIOA_MODER |= (1U << 10);
  GPIOA_ODR &= ~(1U << 5);

  // PB3 = D0 output, PB4 = D1 output
  GPIOB_MODER &= ~((3U << 6) | (3U << 8));
  GPIOB_MODER |= ((1U << 6) | (1U << 8));
  GPIOB_ODR &= ~((1U << 3) | (1U << 4));
}

static void uart_init(void) {
  RCC_APB1ENR |= (1U << 17);
  // 9600 baud @ 16 MHz: USARTDIV = 1666.7 → BRR = 0x0683
  USART2_BRR = 0x0683U;
  USART2_CR1 = (1U << 3) | (1U << 13); // TE | UE
}

static void adc_init(void) {
  RCC_APB2ENR |= (1U << 8); // ADC1 clock
  ADC_CCR = (1U << 16);     // ADCPRE /4 → 4 MHz
  ADC1_SMPR2 = (7U << 0);   // SMP ch0 = 480 cycles (max accuracy)
  ADC1_SQR3 = 0U;           // convert channel 0
  ADC1_CR2 = (1U << 0);     // ADON
  delay_ms(2);
}

static uint32_t adc_avg(int n) {
  uint32_t s = 0;
  for (int i = 0; i < n; i++) {
    ADC1_CR2 |= (1U << 30); // SWSTART
    while (!(ADC1_SR & (1U << 1)))
      ; // wait EOC
    s += ADC1_DR & 0xFFFU;
  }
  return s / (uint32_t)n;
}

static void dac_set(uint32_t d1, uint32_t d0) {
  GPIOB_ODR = (GPIOB_ODR & ~((1U << 3) | (1U << 4))) | (d0 << 3) | (d1 << 4);
}

static void led_blink(int n) {
  for (int i = 0; i < n; i++) {
    GPIOA_ODR |= (1U << 5);
    delay_ms(180);
    GPIOA_ODR &= ~(1U << 5);
    delay_ms(180);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Test table
// ═══════════════════════════════════════════════════════════════════════════
typedef struct {
  uint32_t d1, d0;
  const char *label;
  uint32_t exp_mv;
} T;
static const T tests[4] = {
    {0, 0, "0.00 V", 0},
    {0, 1, "0.83 V", 825},
    {1, 0, "1.65 V", 1650},
    {1, 1, "2.48 V", 2475},
};

// ═══════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════
int main(void) {
  gpio_init();
  uart_init();
  adc_init();

  uart_puts("\r\n==========================================\r\n");
  uart_puts("  2-bit R-2R DAC Checker (STM32F446RE)\r\n");
  uart_puts("  PB3=D0  PB4=D1  PA0=VOUT  9600 baud\r\n");
  uart_puts("==========================================\r\n\r\n");

  while (1) {
    uart_puts("D1  D0   Expected    Measured    Status\r\n");
    uart_puts("-----------------------------------------\r\n");

    for (int i = 0; i < 4; i++) {
      dac_set(tests[i].d1, tests[i].d0);
      delay_ms(20); // R-2R settle time

      uint32_t adc = adc_avg(16); // 16-sample average

      uart_puts(" ");
      uart_putc('0' + (char)tests[i].d1);
      uart_puts("   ");
      uart_putc('0' + (char)tests[i].d0);
      uart_puts("   ");
      uart_puts(tests[i].label);
      uart_puts("    ");
      uart_print_v(adc);
      uart_check(adc, tests[i].exp_mv);

      led_blink(i + 1); // 1,2,3,4 blinks = step
      delay_ms(800);
    }

    uart_puts("-----------------------------------------\r\n\r\n");
    delay_ms(3000);
  }
}
