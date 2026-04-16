

#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define GPIOAEN (1U << 0) // GPIOA clock enable
#define GPIOCEN (1U << 2) // GPIOC clock enable

#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_ODR (*((volatile unsigned int *)0x40020014))

#define PA5_MODER_MASK (3U << 10)
#define PA5_MODER_OUT (1U << 10) // 01 = output
#define PA5_ODR (1U << 5)        // ODR bit 5 = PA5

#define PA6_MODER_MASK (3U << 12)
#define PA6_MODER_OUT (1U << 12) // 01 = output
#define PA6_ODR (1U << 6)        // ODR bit 6 = PA6

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

  GPIOA_MODER &= ~PA6_MODER_MASK;
  GPIOA_MODER |= PA6_MODER_OUT;

  GPIOC_MODER &= ~PC13_MODER_MASK;
  GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
}

void SetReady(void) {
  GPIOA_ODR |= PA5_ODR; // PA5 HIGH -> LD2 ON
}

void ClearReady(void) {
  GPIOA_ODR &= ~PA5_ODR; // PA5 LOW -> LD2 OFF
}

void WaitForASLow(void) {
  while (GPIOC_IDR & PC13_IDR)
    ; // loop while HIGH
}

void WaitForASHigh(void) {
  while (!(GPIOC_IDR & PC13_IDR))
    ; // loop while LOW
}

void SetVT(void) {
  GPIOA_ODR |= PA6_ODR; // PA6 HIGH -> external LED ON
}

void ClearVT(void) {
  GPIOA_ODR &= ~PA6_ODR; // PA6 LOW -> external LED OFF
}

int main(void) {
  GPIO_Init();

  while (1) {
    SetReady();      // 1) Ready HIGH (LED2 green ON)
    WaitForASLow();  // 2) wait for button press (AS falls)
    ClearReady();    // 3) Ready LOW (LED2 green OFF)
    delay_ms(10);    // 4) 10ms debounce
    WaitForASHigh(); // 5) wait for button release (AS rises)
    delay_ms(250);   // 6) 250ms A-V delay
    SetVT();         // 7) VT HIGH (external LED ON)
    delay_ms(250);   // 8) 250ms pulse duration
    ClearVT();       // 9) VT LOW (external LED OFF)
  }
}