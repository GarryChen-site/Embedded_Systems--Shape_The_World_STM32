/**
 * @file    main.c  (RAW bare-metal version)
 * @brief   Lab 10 TrafficLight FSM — STM32 F446RE, no HAL
 *
 * -------------------------------------------------------------------------
 * PIN ASSIGNMENT (identical to HAL version)
 *
 *   LED outputs (220Ω to GND):
 *     EW_RED    PB5  (D4,  CN9)    NS_RED    PA8  (D7, CN9)
 *     EW_YELLOW PB4  (D5,  CN9)    NS_YELLOW PA9  (D8, CN9)
 *     EW_GREEN  PB10 (D6,  CN9)    NS_GREEN  PC7  (D9, CN9)
 *     WALK      PB6  (D10, CN9)    DONTWALK  PA7  (D11,CN9)
 *
 *   Sensor inputs (10kΩ pull-down to GND; switch from pin to 3.3V):
 *     EW_SENSOR  PA6  (D12, CN9)
 *     NS_SENSOR  PA10 (D2,  CN9)
 *     PED_SENSOR PB3  (D3,  CN9)   NOTE: PB3 = SWO pin — works only when
 *                                   SWO trace is disabled in debug config.
 *                                   Move to PB8(D15) if SWO is needed.
 *
 * -------------------------------------------------------------------------
 */

#include <stdint.h>

/* =========================================================================
 * Bare-metal register definitions
 * ========================================================================= */

/* RCC */
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))

/* GPIOA  */
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_PUPDR (*((volatile uint32_t *)0x4002000CU))
#define GPIOA_IDR (*((volatile uint32_t *)0x40020010U))
#define GPIOA_BSRR (*((volatile uint32_t *)0x40020018U))

/* GPIOB */
#define GPIOB_MODER (*((volatile uint32_t *)0x40020400U))
#define GPIOB_PUPDR (*((volatile uint32_t *)0x4002040CU))
#define GPIOB_IDR (*((volatile uint32_t *)0x40020410U))
#define GPIOB_BSRR (*((volatile uint32_t *)0x40020418U))

/* GPIOC */
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_BSRR (*((volatile uint32_t *)0x40020818U))

/* SysTick */
#define STK_CTRL (*((volatile uint32_t *)0xE000E010U))
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U))
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))

/* =========================================================================
 * GPIO MODER encoding (2 bits per pin)
 *   00 = input  01 = output  10 = AF  11 = analog
 * ========================================================================= */
#define PIN_INPUT(n) (0U)
#define PIN_OUTPUT(n) (1U << ((n) * 2U))

/* =========================================================================
 * BSRR encoding
 *   bits[15: 0] = BS (bit set):   write 1 to set pin HIGH
 *   bits[31:16] = BR (bit reset): write 1 to set pin LOW
 * ========================================================================= */
#define BSRR_SET(n) (1U << (n))         /* drive pin HIGH */
#define BSRR_CLR(n) (1U << ((n) + 16U)) /* drive pin LOW  */

/* Write a single output pin to 0 or 1 using BSRR (atomic, glitch-free) */
#define WRITE_PIN(BSRR_REG, n, val)                                            \
  ((BSRR_REG) = ((val) ? BSRR_SET(n) : BSRR_CLR(n)))

/* =========================================================================
 * SysTick configuration
 *
 * HSI clock = 16 MHz (no PLL, matches SystemClock_Config in HAL version).
 * 1 ms = 16 000 core cycles.
 * RELOAD = 16 000 - 1 = 15 999 = 0x3E7F
 *
 * Derivation:
 *   Original TM4C: RELOAD = 800 000 - 1 at 80 MHz → 10 ms per wait unit
 *   STM32:         RELOAD =  16 000 - 1 at 16 MHz → 10 ms per wait unit  ✓
 *
 * STK_CTRL bits used:
 *   bit 2 CLKSOURCE = 1 → processor (AHB) clock
 *   bit 0 ENABLE    = 1 → counter running
 *   bit16 COUNTFLAG   → set when counter reaches 0; cleared on read
 * ========================================================================= */
#define STK_1MS_RELOAD (16000U - 1U)

static void SysTick_Init(void) {
  STK_CTRL = 0U;          /* disable before configuring */
  STK_CTRL = (1U << 2)    /* CLKSOURCE = processor clock */
             | (1U << 0); /* ENABLE */
}

/* Delay n milliseconds by busy-waiting on SysTick COUNTFLAG */
static void SysTick_Wait1ms(uint32_t n) {
  uint32_t i;
  for (i = 0U; i < n; i++) {
    STK_LOAD = STK_1MS_RELOAD;
    STK_VAL = 0U; /* any write clears the counter */
    while ((STK_CTRL & (1U << 16)) == 0U)
      ; /* spin until COUNTFLAG */
  }
}

/* Original SysTick_Wait(N) = N × 10 ms */
static void SysTick_Wait(uint32_t delay) {
  uint32_t i;
  for (i = 0U; i < delay; i++)
    SysTick_Wait1ms(10U);
}

/* =========================================================================
 * FSM type
 * time_ms stored directly
 * ========================================================================= */
typedef struct {
  uint32_t time_ms; /* state duration in ms                               */
  uint8_t traffic;  /* traffic bitmask: bit5=EWR bit4=EWY bit3=EWG        */
                    /*                  bit2=NSR bit1=NSY bit0=NSG        */
  uint8_t walk;     /* walk bitmask:    bit3=WALK(green) bit1=DONTWALK    */
  uint8_t next[8];  /* next state indexed by sensor reading [0..7]         */
} State_t;

/* =========================================================================
 * FSM TABLE
 *
 * Sensor index (bit2=PED, bit1=NS_CAR, bit0=EW_CAR):
 *   next[0] = nobody   next[1] = EW only   next[2] = NS only  next[3] = EW+NS
 *   next[4] = PED only next[5] = PED+EW    next[6] = PED+NS   next[7] = all
 * ========================================================================= */

static const State_t FSM[11] = {
    {200, 0x0C, 0x02, {0, 0, 1, 1, 1, 1, 1, 1}},      /* S0: EW green  */
    {300, 0x14, 0x02, {1, 0, 2, 2, 4, 4, 2, 2}},      /* S1: EW yellow */
    {200, 0x21, 0x02, {2, 3, 2, 3, 3, 3, 3, 3}},      /* S2: NS green  */
    {300, 0x22, 0x02, {3, 0, 2, 0, 4, 0, 4, 4}},      /* S3: NS yellow */
    {200, 0x24, 0x08, {4, 5, 5, 5, 4, 5, 5, 5}},      /* S4: WALK      */
    {50, 0x24, 0x00, {4, 6, 6, 6, 4, 6, 6, 6}},       /* S5: flash off */
    {50, 0x24, 0x02, {4, 7, 7, 7, 4, 7, 7, 7}},       /* S6: flash on  */
    {50, 0x24, 0x00, {4, 8, 8, 8, 4, 8, 8, 8}},       /* S7: flash off */
    {50, 0x24, 0x02, {4, 9, 9, 9, 4, 9, 9, 9}},       /* S8: flash on  */
    {50, 0x24, 0x00, {4, 10, 10, 10, 4, 10, 10, 10}}, /* S9: flash off */
    {50, 0x24, 0x02, {5, 0, 2, 0, 4, 0, 2, 0}},       /* S10:flash on  */
};

/* =========================================================================
 * GPIO_Init
 *
 * Enables clocks and configures all 11 GPIO pins.
 *
 * Clock enable (RCC_AHB1ENR):
 *   bit0 = GPIOAEN   bit1 = GPIOBEN   bit2 = GPIOCEN
 *
 * Output pins (MODER = 01):
 *   GPIOB: PB4(EWY) PB5(EWR) PB6(WALK) PB10(EWG)
 *   GPIOA: PA7(DW)  PA8(NSR) PA9(NSY)
 *   GPIOC: PC7(NSG)
 *
 * Input pins (MODER = 00, no pull — external 10kΩ to GND):
 *   GPIOA: PA6(EW_SENSOR) PA10(NS_SENSOR)
 *   GPIOB: PB3(PED_SENSOR)
 * ========================================================================= */
static void GPIO_Init(void) {
  /* 1. Enable peripheral clocks */
  RCC_AHB1ENR |= (1U << 0) | (1U << 1) | (1U << 2); /* GPIOA, B, C */

  /* ---- GPIOB --------------------------------------------------------- */
  /* Clear bits for PB3, PB4, PB5, PB6, PB10 then set outputs */
  GPIOB_MODER &= ~((3U << 6) |  /* PB3  clear (keep as input = 00) */
                   (3U << 8) |  /* PB4  clear */
                   (3U << 10) | /* PB5  clear */
                   (3U << 12) | /* PB6  clear */
                   (3U << 20)); /* PB10 clear */
  GPIOB_MODER |= ((1U << 8) |   /* PB4  → output */
                  (1U << 10) |  /* PB5  → output */
                  (1U << 12) |  /* PB6  → output */
                  (1U << 20));  /* PB10 → output */
  GPIOB_PUPDR &= ~(3U << 6);    /* PB3: no pull-up/down */

  /* ---- GPIOA --------------------------------------------------------- */
  /* Clear PA6, PA7, PA8, PA9, PA10 bits */
  GPIOA_MODER &= ~((3U << 12) | /* PA6  clear (keep as input) */
                   (3U << 14) | /* PA7  clear */
                   (3U << 16) | /* PA8  clear */
                   (3U << 18) | /* PA9  clear */
                   (3U << 20)); /* PA10 clear (keep as input) */
  GPIOA_MODER |= ((1U << 14) |  /* PA7  → output (DONTWALK) */
                  (1U << 16) |  /* PA8  → output (NS_RED) */
                  (1U << 18));  /* PA9  → output (NS_YELLOW) */
  GPIOA_PUPDR &= ~((3U << 12) | /* PA6:  no pull */
                   (3U << 20)); /* PA10: no pull */

  /* ---- GPIOC --------------------------------------------------------- */
  GPIOC_MODER &= ~(3U << 14); /* PC7  clear */
  GPIOC_MODER |= (1U << 14);  /* PC7  → output (NS_GREEN) */

  /* Default all LED outputs LOW at startup */
  GPIOB_BSRR = BSRR_CLR(4) | BSRR_CLR(5) | BSRR_CLR(6) | BSRR_CLR(10);
  GPIOA_BSRR = BSRR_CLR(7) | BSRR_CLR(8) | BSRR_CLR(9);
  GPIOC_BSRR = BSRR_CLR(7);
}

/* =========================================================================
 * apply_traffic
 *
 * Drives the 8 LED GPIO pins from traffic and walk bitmasks.
 * Uses BSRR (atomic set/clear) so each pin is driven in a single write.
 *
 * Traffic bitmask:                Walk bitmask:
 *   bit5 → EW_RED  (PB5)           bit3 → WALK     (PB6)
 *   bit4 → EW_YELLOW (PB4)         bit1 → DONTWALK (PA7)
 *   bit3 → EW_GREEN (PB10)
 *   bit2 → NS_RED  (PA8)
 *   bit1 → NS_YELLOW (PA9)
 *   bit0 → NS_GREEN (PC7)
 *
 * BSRR strategy: build a 32-bit word where bits[15:0] = pins to SET HIGH
 * and bits[31:16] = pins to SET LOW, then write it once per GPIO port.
 * ========================================================================= */
static void apply_traffic(uint8_t traffic, uint8_t walk) {
  uint32_t bsrr_b = 0U;
  uint32_t bsrr_a = 0U;
  uint32_t bsrr_c = 0U;

  /* --- GPIOB: EW_RED(PB5), EW_YELLOW(PB4), EW_GREEN(PB10), WALK(PB6) - */
  bsrr_b |= ((traffic >> 5) & 1U) ? BSRR_SET(5) : BSRR_CLR(5);   /* EW_RED */
  bsrr_b |= ((traffic >> 4) & 1U) ? BSRR_SET(4) : BSRR_CLR(4);   /* EW_YEL */
  bsrr_b |= ((traffic >> 3) & 1U) ? BSRR_SET(10) : BSRR_CLR(10); /* EW_GRN */
  bsrr_b |= ((walk >> 3) & 1U) ? BSRR_SET(6) : BSRR_CLR(6);      /* WALK   */
  GPIOB_BSRR = bsrr_b;

  /* --- GPIOA: NS_RED(PA8), NS_YELLOW(PA9), DONTWALK(PA7) -------------- */
  bsrr_a |= ((traffic >> 2) & 1U) ? BSRR_SET(8) : BSRR_CLR(8); /* NS_RED */
  bsrr_a |= ((traffic >> 1) & 1U) ? BSRR_SET(9) : BSRR_CLR(9); /* NS_YEL */
  bsrr_a |= ((walk >> 1) & 1U) ? BSRR_SET(7) : BSRR_CLR(7);    /* DONTWLK*/
  GPIOA_BSRR = bsrr_a;

  /* --- GPIOC: NS_GREEN(PC7) ------------------------------------------- */
  bsrr_c |= ((traffic >> 0) & 1U) ? BSRR_SET(7) : BSRR_CLR(7); /* NS_GRN */
  GPIOC_BSRR = bsrr_c;
}

/* =========================================================================
 * read_sensors
 *
 * Reads three GPIO inputs and assembles the 3-bit FSM index.
 * Returns: bit2=PED_SENSOR(PB3)  bit1=NS_SENSOR(PA10)  bit0=EW_SENSOR(PA6)
 * Active HIGH: switch to 3.3V with 10kΩ pull-down → pressed = 1.
 * ========================================================================= */
static uint8_t read_sensors(void) {
  uint8_t s = 0U;
  if (GPIOA_IDR & (1U << 6))
    s |= (1U << 0); /* EW_SENSOR  → bit0 */
  if (GPIOA_IDR & (1U << 10))
    s |= (1U << 1); /* NS_SENSOR  → bit1 */
  if (GPIOB_IDR & (1U << 3))
    s |= (1U << 2); /* PED_SENSOR → bit2 */
  return s;         /* 0..7 */
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void) {
  uint8_t state = 0U; /* FSM starts in state 0: EW green, NS red */

  GPIO_Init();
  SysTick_Init();

  while (1) {
    apply_traffic(FSM[state].traffic, FSM[state].walk); /* drive LEDs    */
    SysTick_Wait(FSM[state].time_ms / 10U);             /* hold state    */
    state = FSM[state].next[read_sensors()];            /* advance FSM   */
  }
}
