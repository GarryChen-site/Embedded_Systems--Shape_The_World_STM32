// i2c.c — Lab 14 ADC, STM32 F446RE
//
// Bare-metal I2C1 driver: polling, fast mode (400 kHz), master-transmit only.
// Uses PB8 (SCL) and PB9 (SDA) in open-drain AF4 mode.
//
// CLOCK DERIVATION (PCLK1 = 16 MHz, HSI, no PLL):
//   Fast mode (Fscl ≤ 400 kHz), DUTY=0 → Tlow = 2×Thigh
//   CCR   = PCLK1 / (3 × Fscl) = 16 MHz / (3 × 400 kHz) = 13.33 → 14
//   Fscl  = 16 MHz / (3 × 14)  = 380 kHz  (< 400 kHz ✓)
//   TRISE = floor(PCLK1 × Tr_max) + 1
//         = floor(16 MHz × 300 ns) + 1 = floor(4.8) + 1 = 6
//
// REGISTER MAP:
//   I2C1 base 0x40005400
//   RCC APB1ENR bit 21 = I2C1EN
//   GPIOB AFRH: PB8=bits[3:0]=4, PB9=bits[7:4]=4  (AF4 = I2C1)
//

#include "i2c.h"
#include <stdint.h>

// =========================================================================
// RCC registers
// =========================================================================
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U)) // GPIO clocks
#define RCC_APB1ENR (*((volatile uint32_t *)0x40023840U)) // I2C1 clock

// =========================================================================
// GPIOB registers
// =========================================================================
#define GPIOB_MODER (*((volatile uint32_t *)0x40020400U))
#define GPIOB_OTYPER (*((volatile uint32_t *)0x40020404U))
#define GPIOB_OSPEEDR (*((volatile uint32_t *)0x40020408U))
#define GPIOB_PUPDR (*((volatile uint32_t *)0x4002040CU))
#define GPIOB_AFRH (*((volatile uint32_t *)0x40020424U)) // AF for PB8–PB15

// =========================================================================
// I2C1 registers
// =========================================================================
#define I2C1_CR1 (*((volatile uint32_t *)0x40005400U))
#define I2C1_CR2 (*((volatile uint32_t *)0x40005404U))
#define I2C1_DR (*((volatile uint32_t *)0x40005410U))
#define I2C1_SR1 (*((volatile uint32_t *)0x40005414U))
#define I2C1_SR2 (*((volatile uint32_t *)0x40005418U))
#define I2C1_CCR (*((volatile uint32_t *)0x4000541CU))
#define I2C1_TRISE (*((volatile uint32_t *)0x40005420U))

// SR1 flag bits
#define I2C_SR1_SB (1U << 0)   // Start bit generated
#define I2C_SR1_ADDR (1U << 1) // Address sent / matched
#define I2C_SR1_BTF (1U << 2)  // Byte transfer finished
#define I2C_SR1_TXE (1U << 7)  // Transmit data register empty
#define I2C_SR1_AF (1U << 10)  // Acknowledge failure

// CR1 control bits
#define I2C_CR1_PE (1U << 0)     // Peripheral enable
#define I2C_CR1_START (1U << 8)  // Start generation
#define I2C_CR1_STOP (1U << 9)   // Stop generation
#define I2C_CR1_SWRST (1U << 15) // Software reset

// SR2 flag bits
#define I2C_SR2_BUSY (1U << 1) // Bus busy

// Polling timeout (prevents infinite loop on I2C fault)
#define I2C_TIMEOUT 50000U

// =========================================================================
// i2c1_recover — software-reset I2C1 if it gets stuck in BUSY state
// =========================================================================
static void i2c1_recover(void) {
  I2C1_CR1 &= ~I2C_CR1_PE;     // disable peripheral
  I2C1_CR1 |= I2C_CR1_SWRST;   // assert software reset
  I2C1_CR1 &= ~I2C_CR1_SWRST;  // deassert reset
  I2C1_CR2 = 16U;              // restore FREQ = 16 (PCLK1 MHz)
  I2C1_CCR = (1U << 15) | 14U; // fast mode, CCR=14
  I2C1_TRISE = 6U;
  I2C1_CR1 = I2C_CR1_PE; // re-enable
}

// =========================================================================
// I2C1_Init
// =========================================================================
void I2C1_Init(void) {
  // 1. Enable peripheral clocks
  RCC_AHB1ENR |= (1U << 1);  // GPIOBEN
  RCC_APB1ENR |= (1U << 21); // I2C1EN
  volatile uint32_t dummy = RCC_APB1ENR;
  (void)dummy; // clock settle

  // 2. Configure PB8 (SCL) and PB9 (SDA)
  //    MODER = 10 (Alternate Function) — bits [17:16] for PB8, [19:18] for PB9
  GPIOB_MODER &= ~((3U << 16) | (3U << 18));
  GPIOB_MODER |= ((2U << 16) | (2U << 18));

  //    OTYPER = 1 (open-drain) — bit 8 for PB8, bit 9 for PB9
  GPIOB_OTYPER |= (1U << 8) | (1U << 9);

  //    OSPEEDR = 11 (very high speed) — bits [17:16], [19:18]
  GPIOB_OSPEEDR |= (3U << 16) | (3U << 18);

  //    PUPDR = 00 (no pull) — module has on-board pull-ups
  GPIOB_PUPDR &= ~((3U << 16) | (3U << 18));

  //    AFRH: AF4 for PB8 (bits [3:0]) and PB9 (bits [7:4])
  GPIOB_AFRH &= ~((0xFU << 0) | (0xFU << 4));
  GPIOB_AFRH |= ((4U << 0) | (4U << 4));

  // 3. Configure I2C1
  I2C1_CR1 = 0; // disable PE while configuring

  //    CR2: FREQ = PCLK1 in MHz = 16
  I2C1_CR2 = 16U;

  //    CCR: fast mode (FS=1), DUTY=0, CCR=14  → Fscl ≈ 380 kHz
  I2C1_CCR = (1U << 15) | 14U;

  //    TRISE: max SCL rise time / T_PCLK1 + 1 = 300ns/62.5ns + 1 ≈ 6
  I2C1_TRISE = 6U;

  I2C1_CR1 = I2C_CR1_PE; // enable peripheral
}

// =========================================================================
// i2c1_start_write — generate START and send 7-bit slave address (write)
// Returns 0 on success, 1 on timeout
// =========================================================================
static int i2c1_start_write(uint8_t addr7) {
  uint32_t t;

  // Wait for bus to be free
  t = I2C_TIMEOUT;
  while ((I2C1_SR2 & I2C_SR2_BUSY) && --t)
    ;
  if (!t) {
    i2c1_recover();
    return 1;
  }

  // Generate START
  I2C1_CR1 |= I2C_CR1_START;
  t = I2C_TIMEOUT;
  while (!(I2C1_SR1 & I2C_SR1_SB) && --t)
    ;
  if (!t)
    return 1;

  // Send slave address with W=0 (clears SB)
  I2C1_DR = (uint8_t)((addr7 << 1) & 0xFEU);
  t = I2C_TIMEOUT;
  while (!(I2C1_SR1 & I2C_SR1_ADDR) && --t) {
    if (I2C1_SR1 & I2C_SR1_AF) { // NACK → abort
      I2C1_SR1 &= ~I2C_SR1_AF;
      I2C1_CR1 |= I2C_CR1_STOP;
      return 1;
    }
  }
  if (!t)
    return 1;

  (void)I2C1_SR2; // read SR2 to clear ADDR flag
  return 0;
}

// =========================================================================
// i2c1_write_byte — wait for TXE and write one byte
// =========================================================================
static void i2c1_write_byte(uint8_t b) {
  uint32_t t = I2C_TIMEOUT;
  while (!(I2C1_SR1 & I2C_SR1_TXE) && --t)
    ;
  I2C1_DR = b;
}

// =========================================================================
// i2c1_stop — wait for BTF then generate STOP
// =========================================================================
static void i2c1_stop(void) {
  uint32_t t = I2C_TIMEOUT;
  while (!(I2C1_SR1 & I2C_SR1_BTF) && --t)
    ;
  I2C1_CR1 |= I2C_CR1_STOP;
}

// =========================================================================
// I2C1_Send
// =========================================================================
void I2C1_Send(uint8_t addr7, const uint8_t *buf, uint32_t len) {
  if (i2c1_start_write(addr7))
    return;
  for (uint32_t i = 0; i < len; i++)
    i2c1_write_byte(buf[i]);
  i2c1_stop();
}

// =========================================================================
// I2C1_SendFrag
// =========================================================================
void I2C1_SendFrag(uint8_t addr7, uint8_t pre, const uint8_t *data,
                   uint32_t len) {
  if (i2c1_start_write(addr7))
    return;
  i2c1_write_byte(pre);
  for (uint32_t i = 0; i < len; i++)
    i2c1_write_byte(data[i]);
  i2c1_stop();
}
