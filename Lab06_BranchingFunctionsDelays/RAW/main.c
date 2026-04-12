

#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define GPIOAEN (1U << 0) // GPIOA clock enable
#define GPIOCEN (1U << 2) // GPIOC clock enable

#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_ODR (*((volatile unsigned int *)0x40020014))

#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10) // 01 = output
#define PA5_ODR (1U << 5)        // ODR bit 5 = PA5

#define GPIOC_MODER (*((volatile unsigned int *)0x40020800))
#define GPIOC_PUPDR (*((volatile unsigned int *)0x4002080C))
#define GPIOC_IDR (*((volatile unsigned int *)0x40020810))

#define PC13_MODER_MASK (3U << 26)
#define PC13_PUPDR_MASK (3U << 26)
#define PC13_IDR (1U << 13)

void delay_ms(volatile unsigned int ms) {
  volatile unsigned int count = ms * 1600;
  while (count > 0) {
    count--;
  }
}

void GPIO_Init(void) {

  RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);
  volatile unsigned int dummy = RCC_AHB1ENR;
  (void)dummy;

  GPIOA_MODER &= ~PA5_MODER_MASK;
  GPIOA_MODER |= PA5_MODER_OUT;

  GPIOC_MODER &= ~PC13_MODER_MASK;
  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
}

unsigned int input;
unsigned int output;

int main(void) {
  GPIO_Init();

  GPIOA_ODR |= PA5_ODR;

  while (1) {
    delay_ms(100);

    input = GPIOC_IDR & PC13_IDR;

    if (input == 0x00) {
      GPIOA_ODR ^= PA5_ODR;
    } else {
      GPIOA_ODR |= PA5_ODR;
    }

    output = GPIOA_ODR & PA5_ODR;
  }
}