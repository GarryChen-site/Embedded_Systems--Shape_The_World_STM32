
#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define GPIOAEN (1U << 0) // GPIOA clock enable
#define GPIOCEN (1U << 2) // GPIOC clock enable

// =========================================================================
// GPIOA — PA6 = buzzer output (active HIGH)
// =========================================================================
#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_ODR (*((volatile unsigned int *)0x40020014))
#define PA6_MODER_MASK (3U << 12)
#define PA6_MODER_OUT (1U << 12)
#define PA6_ODR (1U << 6)

// =========================================================================
// GPIOC — PC13 = B1 button (active LOW: 0=pressed, 1=released)
// =========================================================================
#define GPIOC_MODER (*((volatile unsigned int *)0x40020800))
#define GPIOC_IDR (*((volatile unsigned int *)0x40020810))
#define PC13_MODER_MASK (3U << 26)
#define PC13_IDR (1U << 13)

#define SW_PRESSED ((GPIOC_IDR & PC13_IDR) == 0)

// =========================================================================
// SysTick timer registers — 24-bit auto-reload timer, clock from AHB
// =========================================================================
#define STK_CTRL (*((volatile unsigned int *)0xE000E010))
#define STK_LOAD (*((volatile unsigned int *)0xE000E014))
#define STK_VAL (*((volatile unsigned int *)0xE000E018))

// SysTick priority register
#define SCB_SHPR3 (*((volatile unsigned int *)0xE000ED20))

// each half-period requires one ISR call
// ISR frequency = 2 * output frequency = 880 Hz
// 18182 distinct values (including 0)
// RELOAD = (16MHz / 880Hz) - 1 = 18181
#define RELOAD_440HZ 18181U // 16MHz/18182 = 880Hz ISR → 440Hz output

volatile unsigned int forkOn;
volatile unsigned int input_sw;

void Sound_Init(void) {
  RCC_AHB1ENR |= GPIOAEN | GPIOCEN;
  volatile unsigned int dummy = RCC_AHB1ENR;
  (void)dummy;

  GPIOA_MODER &= ~PA6_MODER_MASK;
  GPIOA_MODER |= PA6_MODER_OUT;
  GPIOA_ODR &= ~PA6_ODR;

  GPIOC_MODER &= ~PC13_MODER_MASK;

  STK_CTRL = 0;
  STK_LOAD = RELOAD_440HZ;
  STK_VAL = 0;
  SCB_SHPR3 &= 0x00FFFFFF;

  STK_CTRL = 0x00000007;
}

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

  while (1) {
  }
}