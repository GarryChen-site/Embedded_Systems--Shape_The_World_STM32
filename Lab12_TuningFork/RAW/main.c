// main.c — Lab 12 TuningFork, STM32 F446RE Nucleo, bare-metal
//
// Original: 440 Hz square wave on PA2, switch on PA3 (TM4C, active HIGH)
// Ported to: 440 Hz square wave on PA6, B1 button on PC13 (active LOW)
//
// Buzzer spec: resonant at 6 kHz, capacitance 15000 pF, 300Ω impedance
//   ⚠ At 440 Hz the buzzer is OFF resonance — it behaves as a 15000 pF
//     capacitor with Xc = 1/(2π×440×15000pF) ≈ 24 kΩ.
//     Current at 440 Hz: I = 3.3V / 24kΩ ≈ 0.14 mA — very quiet but audible.
//     Add a 1kΩ series resistor as the original lab specifies (limits current
//     and makes the voltage divider well-defined).
//
// Wiring:
//   PA6 (CN10-13) ──1kΩ── Buzzer+ ── Buzzer− ── GND
//   PC13 = built-in B1 button (active LOW, external pull-up on board)
//
// SysTick timing at 16 MHz HSI:
//   Output frequency = 440 Hz → ISR fires at 880 Hz (toggles each call)
//   RELOAD = 16,000,000 / 880 − 1 = 18,181
//   Actual = 16,000,000 / 18,182 = 879.97 Hz → 439.98 Hz ✓
//
// Toggle logic (identical state machine as original):
//   Press  #1 → forkOn=1 (sound ON)
//   Release#1 → forkOn stays 1
//   Press  #2 → forkOn=0 (sound OFF)
//   Release#2 → forkOn stays 0
//

#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOAEN (1U << 0) // GPIOA clock enable
#define GPIOCEN (1U << 2) // GPIOC clock enable

// =========================================================================
// GPIOA — PA6 = buzzer output (active HIGH)
// =========================================================================
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014))
#define PA6_MODER_MASK (3U << 12)
#define PA6_MODER_OUT (1U << 12)
#define PA6_ODR (1U << 6)

// =========================================================================
// GPIOC — PC13 = B1 button (active LOW: 0=pressed, 1=released)
// =========================================================================
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))
#define PC13_MODER_MASK (3U << 26)
#define PC13_IDR (1U << 13)

#define SW_PRESSED ((GPIOC_IDR & PC13_IDR) == 0)

// =========================================================================
// SysTick timer registers — 24-bit auto-reload timer, clock from AHB
// =========================================================================
#define STK_CTRL (*((volatile uint32_t *)0xE000E010U))
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U))
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))

// SysTick priority register
#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20U))

// each half-period requires one ISR call
// ISR frequency = 2 * output frequency = 880 Hz
// 18182 distinct values (including 0)
// RELOAD = (16MHz / 880Hz) - 1 = 18181
#define RELOAD_440HZ 18181U // 16MHz/18182 = 880Hz ISR → 440Hz output

volatile uint32_t forkOn;
volatile uint32_t input_sw;

void Sound_Init(void) {
  // 1. Enable GPIO clocks
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // 2. PA6 → output (buzzer)
  GPIOA_MODER &= ~PA6_MODER_MASK;
  GPIOA_MODER |= PA6_MODER_OUT;
  GPIOA_ODR &= ~PA6_ODR; // start silent

  // 2b. PA5 → output (LD2 green LED — visual mirror of forkOn)
  GPIOA_MODER &= ~(3U << 10); // clear MODER bits [11:10]
  GPIOA_MODER |= (1U << 10);  // 01 = output
  GPIOA_ODR &= ~(1U << 5);    // start OFF

  // 3. PC13 → input (B1 button; no PUPDR change — external pull-up on board)
  GPIOC_MODER &= ~PC13_MODER_MASK; // 00 = input

  // 4. SysTick interrupt at 12 kHz → 6 kHz square wave
  STK_CTRL = 0;             // disable during setup
  STK_LOAD = RELOAD_440HZ;  // 880 Hz ISR → 440 Hz square wave
  STK_VAL = 0;              // clear current
  SCB_SHPR3 &= 0x00FFFFFFU; // SysTick priority = 0 (highest)
                            // original: NVIC_SYS_PRI3_R & 0x00FFFFFF
  STK_CTRL = 0x00000007U;   // ENABLE | TICKINT | CLKSOURCE(AHB)
}

// =========================================================================
// SysTick_Handler
// Called at 12 kHz. Detects switch edges and toggles buzzer.
//
// State machine (mirrors original exactly, active-LOW adapted via SW_PRESSED):
//   input_sw=0, SW=0: idle
//   input_sw=0 → SW rises (press #1): forkOn=1, input_sw=1
//   input_sw=1 → SW falls (release):  input_sw=2
//   input_sw=2 → SW rises (press #2): forkOn=0, input_sw=3
//   input_sw=3 → SW falls (release):  input_sw=0
// =========================================================================
void SysTick_Handler(void) {
  int sw = SW_PRESSED;

  if (sw && !input_sw) {
    forkOn = 1;
    input_sw = 1;
  } else if (sw && input_sw == 2) {
    forkOn = 0;
    input_sw = 3;
  } else if (!sw && input_sw == 1) {
    forkOn = 1;
    input_sw = 2;
  } else if (!sw && input_sw == 3) {
    forkOn = 0;
    input_sw = 0;
  }

  if (forkOn) {
    GPIOA_ODR ^= PA6_ODR;
  } else {
    GPIOA_ODR &= ~PA6_ODR;
  }
}

int main(void) {
  forkOn = 0;
  input_sw = 0;
  Sound_Init();
  while (1) {
    // Mirror forkOn → LD2 (PA5) for visual feedback during physical testing
    if (forkOn) {
      GPIOA_ODR |= (1U << 5);
    } else {
      GPIOA_ODR &= ~(1U << 5);
    }
  }
}