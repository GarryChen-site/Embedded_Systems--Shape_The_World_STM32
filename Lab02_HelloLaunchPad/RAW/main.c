// Lab 2 HelloLaunchPad — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX (Tiva LaunchPad)
// Ported to: STM32 F446RE Nucleo board
//
// -------------------------------------------------------------------------
// ORIGINAL BEHAVIOR (TM4C123GX):
//   SW1 (PF4, active LOW) NOT pressed  → LED toggles blue  ↔ red   (100ms each)
//   SW1 (PF4, active LOW)     pressed  → LED toggles blue  ↔ green  (100ms
//   each)
//
// ADAPTED BEHAVIOR (STM32 F446RE — single green LED LD2 on PA5):
//   B1 (PC13, active LOW) NOT pressed  → LED blinks FAST (100ms ON, 100ms OFF)
//                                         simulates blue ↔ red
//   B1 (PC13, active LOW)     pressed  → LED blinks SLOW (100ms ON, 400ms OFF)
//                                         simulates blue ↔ green
//
// ADAPTATION REASON:
//   The Nucleo board has only one user LED (LD2, green, PA5).
//   The TM4C LaunchPad had an RGB LED on PF1/PF2/PF3.
//   Blink speed is used to preserve the two distinguishable states.
// -------------------------------------------------------------------------

#include <stdint.h>

// =========================================================================
// REGISTER DEFINITIONS  (bare-metal, no HAL, no CMSIS)
// =========================================================================
//
// How to read these:
//   (*((volatile uint32_t *)0xADDRESS))
//   ↓
//   Dereference the address as a 32-bit volatile memory location.
//   "volatile" tells the compiler: never cache this, always go to hardware.
//   Memory-mapped register access.
// =========================================================================

// -------------------------------------------------------------------------
// RCC — Reset and Clock Control
// Every peripheral on STM32 requires its clock to be enabled before use.
//
// RCC base address: 0x40023800
// RCC_AHB1ENR register offset: 0x30
// -------------------------------------------------------------------------
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))

// RCC_AHB1ENR bit definitions
#define GPIOAEN (1U << 0) // Bit 0: GPIOA clock enable
#define GPIOCEN (1U << 2) // Bit 2: GPIOC clock enable

// -------------------------------------------------------------------------
// GPIOA — PA5 is the user LED (LD2, green, active HIGH)
//
// GPIOA base address: 0x40020000
// -------------------------------------------------------------------------
// GPIO port mode register
// Each GPIO pin uses 2 bits:  00=Input  01=Output  10=AF  11=Analog
// PA5 uses bits [11:10]
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))

// GPIO port output data register
// Each bit directly controls the output state of the corresponding pin.
// Bit 5 → PA5 output value
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

// Masks / values for PA5
#define PA5_MODER_MASK (3U << 10) // 2-bit mask for PA5
#define PA5_MODER_OUT (1U << 10)  // 01 = general-purpose output mode
#define PA5_ODR (1U << 5)         // PA5 is the 5th bit

// -------------------------------------------------------------------------
// GPIOC — PC13 is the user button (B1, active LOW)
//
// GPIOC base address: 0x40020800
// -------------------------------------------------------------------------
// GPIO port mode register
// PC13 uses bits [27:26] — we want 00 (input), which is the reset value
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
// GPIO port pull-up/pull-down register
// Each pin uses 2 bits:  00=None  01=Pull-up  10=Pull-down
// PC13 uses bits [27:26]
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
// GPIO port input data register (read-only)
// Each bit reflects the current state of the corresponding pin.
// Bit 13 → PC13 current level
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

// Masks for PC13
#define PC13_MODER_MASK (3U << 26) // 2-bit mask for PC13
#define PC13_PUPDR_MASK (3U << 26) // 2-bit mask for PC13
#define PC13_IDR (1U << 13)        // 13th bit for PC13

// =========================================================================
// DELAY CONSTANTS
// =========================================================================
//
// Default system clock after reset: 16 MHz HSI (High Speed Internal oscillator)
// The HSI clock signal is generated from an internal 16 MHz RC oscillator.
// The RCC_CFGR register reset value selects HSI as system clock.
//
// Busy-wait loop calibration (Delay function, no optimization / -O0):
//   Each iteration of "while(count) { count--; }" compiles to roughly:
//     LDR r0, [sp]    ; load count    (~1 cycle)
//     SUBS r0, #1     ; decrement     (~1 cycle)
//     STR r0, [sp]    ; store (volatile forces this) (~1 cycle)
//     CBZ / BNE       ; branch        (~1-2 cycles)
//   ≈ 4 cycles per iteration at 16 MHz
//
//   1 second  = 16,000,000 cycles / 4 = 4,000,000 iterations
//   100 ms    = 400,000 iterations
//   400 ms    = 1,600,000 iterations
//
// NOTE: This is an intentional approximation (same philosophy as original lab).
// =========================================================================
#define DELAY_100MS 400000UL
#define DELAY_400MS 1600000UL

void GPIO_Init(void);
void Delay(volatile uint32_t count);

int main(void) {
  // initialize PA5(LED) and PC13 (button)
  GPIO_Init();

  while (1) {
    // Read button state
    // PC13 is active LOW, so if the button is pressed, the value will be 0.
    uint32_t button_pressed = !(GPIOC_IDR & PC13_IDR);

    // Turn LED ON
    // PA5 HIGH → LD2 ON
    GPIOA_ODR |= PA5_ODR;
    Delay(DELAY_100MS);

    // Turn LED OFF
    // PA5 LOW → LD2 OFF
    GPIOA_ODR &= ~PA5_ODR;

    if (button_pressed) {
      // SLOW blink
      Delay(DELAY_400MS);
    } else {
      // FAST blink
      Delay(DELAY_100MS);
    }
  }
}

// =========================================================================
// GPIO_Init
// Initialize PA5 as output (LED) and PC13 as input (button).
// =========================================================================
void GPIO_Init(void) {

  // -----------------------------------------------------------------
  // STEP 1: Enable GPIO peripheral clocks
  // -----------------------------------------------------------------
  // On STM32, ALL peripherals start with their clock DISABLED to save power.
  // You must enable the clock before touching ANY register of that peripheral.
  //
  // Register: RCC_AHB1ENR
  //   Bit 0 (GPIOAEN): 1 = enable GPIOA clock
  //   Bit 2 (GPIOCEN): 1 = enable GPIOC clock
  // -----------------------------------------------------------------
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);

  // -----------------------------------------------------------------
  // STEP 2: Short delay after enabling the clock
  // -----------------------------------------------------------------
  // Reading the register back creates a pipeline stall of a few cycles —
  // enough for the clock to stabilize.
  // -----------------------------------------------------------------
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // -----------------------------------------------------------------
  // STEP 3: Set PA5 as general-purpose output (LED)
  // -----------------------------------------------------------------
  // Register: GPIOA_MODER
  //   PA5 uses bits [11:10]:  00=Input  01=Output  10=AF  11=Analog
  //
  // Pattern: read-modify-write to avoid disturbing other pins.
  //   1. CLEAR bits [11:10] (set to 00 = input, clean slate)
  //   2. SET   bits [11:10] to 01 (output)
  // -----------------------------------------------------------------
  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  // -----------------------------------------------------------------
  // STEP 4: Set PC13 as general-purpose input (button)
  // -----------------------------------------------------------------
  // Register: GPIOC_MODER
  //   PC13 uses bits [27:26]:  00 = input (this IS the reset value)
  //   We clear them explicitly for clarity / defensive programming.
  // -----------------------------------------------------------------
  GPIOC_MODER &= ~PC13_MODER_MASK;

  // -----------------------------------------------------------------
  // STEP 5: Configure pull-up/pull-down for PC13
  // -----------------------------------------------------------------
  // On Nucleo board, B1 (PC13) already has an EXTERNAL 4.7 kΩ pull-up
  // to VDD.
  //
  // Therefore: we disable the internal pull-up (it would form a voltage
  // divider with the external one, which is harmless but unnecessary).
  // We set PUPDR bits [27:26] = 00 (no internal pull).
  //
  // Register: GPIOC_PUPDR
  //   00 = No pull-up/pull-down
  //   01 = Pull-up
  //   10 = Pull-down
  // -----------------------------------------------------------------
  // (00 = none, which is what &= ~mask gives us)
  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
}

// =========================================================================
// Delay
// Busy-wait delay.
//
// =========================================================================
void Delay(volatile uint32_t count) {
  while (count) {
    count--;
  }
}