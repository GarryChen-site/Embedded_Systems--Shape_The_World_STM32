// main.c
// Lab 9 FunctionalDebugging — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX
// Ported to: STM32 F446RE Nucleo board
//
// -------------------------------------------------------------------------
// WHAT THIS LAB TEACHES:
//   Functional debugging by recording I/O state changes into arrays.
//   Instead of connecting an oscilloscope, you snapshot the system's
//   behavior into RAM, then inspect the arrays in the debugger afterward.
//
// BEHAVIOR:
//   - If NEITHER SW1 nor SW2 is pressed → LED stays OFF
//   - If EITHER SW1 or SW2 is pressed   → LED toggles at 10 Hz (50ms on, 50ms
//   off)
//   - Every time input OR output changes → record state in Data[] and Time[]
//
// DEBUGGING ARRAYS (inspect in CubeIDE debugger after running):
//   Data[50]: combined snapshot of SW1, LED, SW2 state at each change
//   Time[50]: SysTick ticks elapsed between each consecutive change
//   No two adjacent elements in Data[] are equal (only changes are recorded).
//
// -------------------------------------------------------------------------
// HARDWARE (STM32 F446RE Nucleo):
//
//   SW1 (built-in B1)   → PC13, active LOW, external 4.7kΩ pull-up on board
//   SW2 (external button) → PC12, active LOW, internal pull-up enabled
//        Wiring: PC12 (CN10 pin 12) → switch → GND (CN10 pin 9)
//        Same wiring as Lab 4; no resistor needed (internal pull-up used)
//   LED (LD2)           → PA5, active HIGH (green)
//
// -------------------------------------------------------------------------
// KEY NEW CONCEPT: SysTick Timer for Timestamping
//   SysTick is a standard ARM Cortex-M peripheral — it lives at the SAME
//   addresses on BOTH TM4C and STM32. This is defined by the ARM architecture,
//   not by the chip vendor. No address change needed!
//
//   TM4C names → STM32 equivalents (same hardware at same address):
//     NVIC_ST_CTRL_R    → STK_CTRL  (0xE000E010)
//     NVIC_ST_RELOAD_R  → STK_LOAD  (0xE000E014)
//     NVIC_ST_CURRENT_R → STK_VAL   (0xE000E018)
//
//   The counter counts DOWN from STK_LOAD to 0, then reloads.
//   We use it as a free-running 24-bit clock for timestamping.
//   Elapsed time: (old_val - new_val) & 0x00FFFFFF  (handles wraparound)
//
// -------------------------------------------------------------------------

#include <stdint.h>

// =========================================================================
// SYSTICK REGISTERS — Standard ARM Cortex-M, same on TM4C and STM32
// =========================================================================
#define STK_CTRL (*((volatile uint32_t *)0xE000E010U)) // Control & status
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U)) // Reload value
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))  // Current value

// STK_CTRL bits (PM0214 §4.4.1):
#define STK_CTRL_ENABLE (1U << 0)    // Counter enable
#define STK_CTRL_TICKINT (1U << 1)   // Exception on countdown (we don't use)
#define STK_CTRL_CLKSOURCE (1U << 2) // 1 = processor clock (AHB), 0 = AHB/8

// =========================================================================
// RCC CLOCK ENABLES
// =========================================================================
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOAEN (1U << 0) // GPIOA clock
#define GPIOCEN (1U << 2) // GPIOC clock

// =========================================================================
// GPIOA — PA5 = LED (LD2, active HIGH)
// =========================================================================
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))
#define GPIOA_IDR (*((volatile uint32_t *)0x40020010U))

#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10) // 01 = output
#define PA5_ODR (1U << 5)        // ODR bit 5 = PA5

// =========================================================================
// GPIOC — PC13 = SW1 (active LOW), PC12 = SW2 (active LOW)
// =========================================================================
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

// PC13 (SW1): MODER=00(input), PUPDR=00(no internal pull, external 4.7kΩ on
// board)
#define PC13_MODER_MASK (3U << 26)
#define PC13_PUPDR_MASK (3U << 26)
#define PC13_IDR (1U << 13)

// PC12 (SW2): MODER=00(input), PUPDR=01(pull-up, active LOW external switch)
#define PC12_MODER_MASK (3U << 24)
#define PC12_PUPDR_MASK (3U << 24)
#define PC12_PUPDR_PU (1U << 24) // 01 = pull-UP
#define PC12_IDR (1U << 12)

// =========================================================================
// DELAY CONSTANT
// =========================================================================
// At 16 MHz (STK_CTRL CLKSOURCE=1 = processor clock):
//   50 ms = 16,000,000 × 0.05 = 800,000 SysTick ticks
// We use SysTick-based delay for accuracy (not a busy-wait loop).
// See Delay() below.
//
// TM4C original: FREQ_10HZ_INTEGER = 160,000 busy-wait iterations @ 80MHz
// STM32 port:    DELAY_50MS_TICKS  = 800,000 SysTick ticks @ 16MHz
#define DELAY_50MS_TICKS 800000UL // SysTick ticks for 50ms at 16 MHz
#define SYSTICK_MAX 0x00FFFFFFUL

// =========================================================================
// GLOBAL VARIABLES (same names as original for debugger consistency)
// =========================================================================
uint32_t
    Data[50]; // state snapshots: SW1[2], LED[1], SW2[0] — changed events only
uint32_t Time[50]; // SysTick ticks between events (first entry is meaningless)
uint32_t Led;      // current LED output state
uint32_t i, last_item, current_item;

// =========================================================================
// FUNCTION PROTOTYPES
// =========================================================================
void GPIO_Init(void);          // replaces PortF_Init()
void SysTick_Init(void);       // same name as original
void Delay(uint32_t ms);       // SysTick-based, replaces busy-wait
void Dump_Debug(void);         // records state change into Data[]/Time[]
void LED_Flash(void);          // toggle LED + delay + record
void LED_Off(void);            // LED off + record
uint32_t switch_pressed(void); // 1 if either SW1 or SW2 is pressed

// =========================================================================
// READ_PORT — compose a "virtual port" byte combining SW1, LED, SW2
//
// Mirrors: GPIO_PORTF_DATA_R & 0x13  in the original
//   PF4 (SW1) → bit 4  of original byte
//   PF1 (LED) → bit 1  of original byte
//   PF0 (SW2) → bit 0  of original byte
//
// Our pins are on different ports, so we compose manually:
//   bit 2: PC13 state (SW1 — active LOW: 0=pressed, 1=released)
//   bit 1: PA5  state (LED — active HIGH: 0=off, 1=on)
//   bit 0: PC12 state (SW2 — active LOW: 0=pressed, 1=released)
// =========================================================================
static inline uint32_t read_port(void) {
  uint32_t sw1 = (GPIOC_IDR & PC13_IDR) ? 1U : 0U; // 1=released, 0=pressed
  uint32_t led = (GPIOA_IDR & PA5_ODR) ? 1U : 0U;  // 1=on,       0=off
  uint32_t sw2 = (GPIOC_IDR & PC12_IDR) ? 1U : 0U; // 1=released, 0=pressed
  return (sw1 << 2) | (led << 1) | sw2;
}

// =========================================================================
// GPIO_Init — replaces PortF_Init() from original
// =========================================================================
void GPIO_Init(void) {
  // Enable clocks
  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  // PA5 → output (LED, LD2 green, active HIGH)
  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  // PC13 → input, no internal pull (external 4.7kΩ pull-up on Nucleo board)
  // TM4C: GPIO_PORTF_PUR_R |= 0x11   (PF4 and PF0 got internal pull-up)
  // STM32: PC13 already has external pull-up → PUPDR=00 (no internal pull)
  GPIOC_MODER &= ~PC13_MODER_MASK; // 00 = input
  GPIOC_PUPDR &= ~PC13_PUPDR_MASK; // 00 = no pull

  // PC12 → input, internal pull-UP (external switch connects to GND)
  // TM4C: GPIO_PORTF_PUR_R |= 0x11   (PF0 got internal pull-up)
  GPIOC_MODER &= ~PC12_MODER_MASK; // 00 = input
  GPIOC_PUPDR &= ~PC12_PUPDR_MASK; // clear first
  GPIOC_PUPDR |= PC12_PUPDR_PU;    // 01 = pull-up
}

// =========================================================================
// SysTick_Init
// Enable SysTick as a free-running 24-bit down-counter at 16 MHz.
// Used for timestamping in Dump_Debug().
//
// TM4C original:               STM32 port:
//   NVIC_ST_CTRL_R = 0         STK_CTRL  = 0             (disable)
//   NVIC_ST_RELOAD_R = 0xFFFFFF STK_LOAD = 0x00FFFFFF    (max period)
//   NVIC_ST_CURRENT_R = 0      STK_VAL   = 0             (clear)
//   NVIC_ST_CTRL_R = 0x05      STK_CTRL  = ENABLE | CLKSOURCE (processor clock)
//
// The registers are at identical addresses — SysTick is standard ARM Cortex-M.
// =========================================================================
void SysTick_Init(void) {
  STK_CTRL = 0;                                    // disable during setup
  STK_LOAD = SYSTICK_MAX;                          // max reload = 0x00FFFFFF
  STK_VAL = 0;                                     // any write clears it
  STK_CTRL = STK_CTRL_ENABLE | STK_CTRL_CLKSOURCE; // enable, processor clock
  // Now STK_VAL counts: 0x00FFFFFF → 0x00FFFFFE → ... → 0x00000000 → reloads
  // Period = 0x01000000 / 16,000,000 Hz ≈ 1.049 seconds per full cycle
}

// =========================================================================
// Delay
// SysTick-based delay in milliseconds. Accurate to ±1 ms.
// At 16 MHz processor clock: 16,000 SysTick ticks per millisecond.
//
// TM4C original used a busy-wait loop (FREQ_10HZ_INTEGER = 160,000 iterations).
// STM32 port uses SysTick for accuracy — unaffected by compiler optimization.
//
// Wraparound handling:
//   SysTick counts DOWN. If counter wraps through 0, the subtraction still
//   works correctly because all values are masked to 24 bits.
//   Proof: start=100, val=0xFFFFF0, elapsed = (100 - 0xFFFFF0) & 0xFFFFFF = 110
// =========================================================================
void Delay(uint32_t ms) {
  uint32_t ticks = ms * 16000UL; // 16,000 ticks per ms at 16 MHz
  uint32_t start = STK_VAL;      // capture start value
  while (((start - STK_VAL) & SYSTICK_MAX) < ticks)
    ;
  // (start - STK_VAL): elapsed ticks (positive even through wrap)
  // masked with 0x00FFFFFF to stay 24-bit
}

// =========================================================================
// Dump_Debug
// Record a state snapshot into Data[i] and Time[i] ONLY if state changed.
// Mirrors the original Dump_Debug() exactly.
//
// Algorithm:
//   1. Read current SysTick value
//   2. Read current port state (SW1, LED, SW2)
//   3. If state is DIFFERENT from last recorded → store in Data[i] and Time[i]
//   4. Advance index i (wraps at 50)
//
// Time[i] = elapsed SysTick ticks since last change (bigger = longer interval).
// HOW TO INSPECT: Pause in CubeIDE debugger → Expressions → add Data and Time.
// =========================================================================
void Dump_Debug(void) {
  current_item = STK_VAL; // read SysTick NOW
  if (i > 0) {
    if (Data[i - 1] != read_port()) {
      // only record on CHANGE
      Time[i] = (last_item - current_item) & SYSTICK_MAX; // elapsed ticks
      Data[i] = read_port(); // current state snapshot
      i++;
      if (i >= 50)
        i = 0; // wrap ring buffer
    }
  } else {
    // First entry: always record (last_item is meaningless, Time[0] is garbage)
    Time[0] = 0;
    Data[0] = read_port();
    i = 1;
  }
  last_item = current_item; // update for next call
}

// =========================================================================
// LED_Flash
// Toggle LED, wait 50ms, record debug snapshot.
// Mirrors the original LED_Flash() which toggled PF1 and called Delay.
// =========================================================================
void LED_Flash(void) {
  GPIOA_ODR ^= PA5_ODR;                // toggle PA5
  Led = (GPIOA_ODR & PA5_ODR) ? 1 : 0; // capture state for inspection
  Dump_Debug();                        // record state change
  Delay(50);                           // wait 50ms (half-period of 10Hz)
}

// =========================================================================
// LED_Off
// Turn LED off and record debug snapshot (if state changed).
// Mirrors the original LED_Off() which cleared PF1.
// =========================================================================
void LED_Off(void) {
  GPIOA_ODR &= ~PA5_ODR; // PA5 LOW → LED OFF
  Led = 0;
  Dump_Debug(); // record (only if state changed)
}

// =========================================================================
// MAIN
// =========================================================================
int main(void) {
  GPIO_Init();
  SysTick_Init();

  i = 0;
  last_item = STK_VAL;

  while (1) {
    if (switch_pressed()) {
      LED_Flash(); // either switch held → toggle LED at 10 Hz
    } else {
      LED_Off(); // no switch → LED stays OFF
      Delay(50); // still pace the loop at 50ms so debug is consistent
    }
  }
}