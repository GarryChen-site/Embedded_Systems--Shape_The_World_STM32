// main.c
// Lab 7 HeartBlock — STM32 F446RE Nucleo Port
//
// Original course: "Embedded Systems: Shape The World" (UTAustinX)
// Original hardware: TI TM4C123GX (Tiva LaunchPad)
// Ported to: STM32 F446RE Nucleo board
//
// -------------------------------------------------------------------------
// PACEMAKER BEHAVIOR (9-step sequence — IDENTICAL to original):
//
//   1. SetReady()       — Ready HIGH (LD2 green ON:  "waiting for atrial beat")
//   2. WaitForASLow()   — wait until button pressed  (atria begins to contract)
//   3. ClearReady()     — Ready LOW  (LD2 green OFF)
//   4. Delay1ms(10)     — 10 ms debounce
//   5. WaitForASHigh()  — wait until button released (atria finishes
//   contracting)
//   6. Delay1ms(250)    — 250 ms A-V delay (time between atrial & ventricular)
//   7. SetVT()          — VT HIGH    (external LED ON: "pulsing ventricles")
//   8. Delay1ms(250)    — 250 ms pulse duration
//   9. ClearVT()        — VT LOW     (external LED OFF)
//   → repeat from step 1
//
// -------------------------------------------------------------------------
// HARDWARE MAPPING  (STM32 F446RE Nucleo):
//
//   AS   (Atrial Sensor input)      → B1 / PC13, active LOW
//          Built-in button; external 4.7 kΩ pull-up on Nucleo board.
//          Pressed (LOW)  = atria contracting
//          Released (HIGH)= atria done
//          UM1724 → "Hardware layout" → "Push-buttons"
//
//   Ready (output, pacemaker ready) → PA5 (built-in LD2, green, active HIGH)
//          HIGH = ready for next atrial beat
//          LOW  = atrial beat detected, waiting
//          UM1724 → "Hardware layout" → "LEDs"
//
//   VT   (Ventricular Trigger output) → PA6 (external LED, active HIGH)
//          HIGH = ventricular pulse (250 ms)
//          LOW  = idle
//          WIRING: PA6 (CN10 pin 13) → 330 Ω → LED anode → LED cathode → GND
//          GND: CN10 pin 9
//          Source: UM1724 → "Hardware layout" → "ST morpho connector" → CN10
//          table
//
// -------------------------------------------------------------------------
// TWO-OUTPUT ADAPTATION:
//   TM4C had PF3 (green, Ready) and PF1 (red, VT) on the same port.
//   STM32 Nucleo has only one built-in LED (PA5).
//   Solution: PA5 = Ready (built-in), PA6 = VT (external, 1 resistor + LED).
//   PA6 shares GPIOA with PA5 → same clock enable (GPIOAEN), no extra cost.
//
// -------------------------------------------------------------------------

#include <stdint.h>

#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOAEN (1U << 0) // GPIOA clock enable
#define GPIOCEN (1U << 2) // GPIOC clock enable

#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10) // 01 = output
#define PA5_ODR (1U << 5)        // ODR bit 5 = PA5

#define PA6_MODER_MASK (3U << 12)
#define PA6_MODER_OUT (1U << 12) // 01 = output
#define PA6_ODR (1U << 6)        // ODR bit 6 = PA6

#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

#define PC13_MODER_MASK (3U << 26)
#define PC13_PUPDR_MASK (3U << 26)
#define PC13_IDR (1U << 13)

#define CYCLE_ONE_MS 4000UL

// =========================================================================
// FUNCTION PROTOTYPES (identical names to original)
// =========================================================================
void GPIO_Init(void);         // replaces PortF_Init()
void Delay1ms(uint32_t msec); // same signature
void WaitForASLow(void);      // wait for button press
void WaitForASHigh(void);     // wait for button release
void SetVT(void);             // PA6 HIGH
void ClearVT(void);           // PA6 LOW
void SetReady(void);          // PA5 HIGH
void ClearReady(void);        // PA5 LOW

int main(void) {
  GPIO_Init();

  while (1) {
    SetReady();      // 1) Ready HIGH (LED2 green ON)
    WaitForASLow();  // 2) wait for button press (AS falls)
    ClearReady();    // 3) Ready LOW (LED2 green OFF)
    Delay1ms(10);    // 4) 10ms debounce
    WaitForASHigh(); // 5) wait for button release (AS rises)
    Delay1ms(250);   // 6) 250ms A-V delay
    SetVT();         // 7) VT HIGH (external LED ON)
    Delay1ms(250);   // 8) 250ms pulse duration
    ClearVT();       // 9) VT LOW (external LED OFF)
  }
}

void GPIO_Init(void) {

  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);
  volatile uint32_t dummy = RCC_AHB1ENR;
  (void)dummy;

  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  GPIOA_MODER &= ~PA6_MODER_MASK;
  GPIOA_MODER |= PA6_MODER_OUT;

  GPIOC_MODER &= ~PC13_MODER_MASK;
  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
}

// =========================================================================
// SetReady / ClearReady — control PA5 (Ready / LD2 green)
// =========================================================================
void SetReady(void) {
  GPIOA_ODR |= PA5_ODR; // PA5 HIGH -> LD2 ON
}

void ClearReady(void) {
  GPIOA_ODR &= ~PA5_ODR; // PA5 LOW -> LD2 OFF
}

// =========================================================================
// WaitForASLow
// Block until AS (PC13) is LOW (button pressed, atria contracting).
// Returns immediately if already LOW.
// =========================================================================
void WaitForASLow(void) {
  while (GPIOC_IDR & PC13_IDR)
    ; // loop while HIGH
}

// =========================================================================
// WaitForASHigh
// Block until AS (PC13) is HIGH (button released, atria done).
// Returns immediately if already HIGH.
// =========================================================================
void WaitForASHigh(void) {
  while (!(GPIOC_IDR & PC13_IDR))
    ; // loop while LOW
}

// =========================================================================
// SetVT / ClearVT — control PA6 (Ventricular Trigger)
// =========================================================================
void SetVT(void) {
  GPIOA_ODR |= PA6_ODR; // PA6 HIGH -> external LED ON
}

void ClearVT(void) {
  GPIOA_ODR &= ~PA6_ODR; // PA6 LOW -> external LED OFF
}

void Delay1ms(uint32_t msec) {
  volatile uint32_t i;
  while (msec > 0) {
    i = CYCLE_ONE_MS;
    while (i > 0) {
      i--;
    }
    msec--;
  }
}
