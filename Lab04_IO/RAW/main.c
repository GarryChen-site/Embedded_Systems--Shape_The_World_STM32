


#define RCC_AHB1ENR     (*((volatile unsigned int *)0x40023830))

#define GPIOAEN         (1 << 0)
#define GPIOCEN         (1 << 2)

#define GPIOA_MODER     (*((volatile unsigned int *)0x40020000))
#define GPIOA_ODR       (*((volatile unsigned int *)0x40020014))


// PA5 bit fields:
// MODER bits [11:10]:  00=Input  01=Output  10=AF  11=Analog  
#define PA5_MODER_MASK  (3 << 10)
#define PA5_MODER_OUT   (1 << 10)  // 01 = output
#define PA5_ODR         (1 << 5)   // ODR bit 5 = PA5  



#define GPIOC_MODER     (*((volatile unsigned int *)0x40020800))
#define GPIOC_PUPDR     (*((volatile unsigned int *)0x4002080C))
#define GPIOC_IDR       (*((volatile unsigned int *)0x40020810))


// PC13 (SW1 — built-in B1):
// MODER bits [27:26]:  00 = input  
// PUPDR bits [27:26]:  00 = none 
// IDR  bit  13:        reads current pin level  
#define PC13_MODER_MASK (3 << 26)
#define PC13_PUPDR_MASK (3 << 26)
#define SW1_IDR         (1 << 13)  // 0 when pressed (active LOW)


// PC12 (SW2 — external button):
// MODER bits [25:24]:  00 = input  
// PUPDR bits [25:24]:  01 = pull-up (internal, since no external R)  
// IDR  bit  12:        reads current pin level  
#define PC12_MODER_MASK (3 << 24)
#define PC12_PUPDR_MASK (3 << 24)
#define PC12_PUPDR_UP   (1 << 24)  // 01 = pull-up
#define SW2_IDR         (1 << 12)  // 0 when pressed (active LOW)


void delay_ms(volatile unsigned int ms) {
    volatile unsigned int count = ms * 1600; 
    while (count > 0) {
        count--;
    }
}

void GPIO_Init(void) {
    RCC_AHB1ENR |= (GPIOAEN | GPIOCEN);

    GPIOA_MODER &= ~PA5_MODER_MASK;
    GPIOA_MODER |= PA5_MODER_OUT;

    GPIOC_MODER &= ~PC13_MODER_MASK;

    GPIOC_MODER &= ~PC12_MODER_MASK;
    
    GPIOC_PUPDR &= ~PC13_PUPDR_MASK;
    GPIOC_PUPDR &= ~PC12_PUPDR_MASK;
    GPIOC_PUPDR |= PC12_PUPDR_UP;

}

int main(void) {
    GPIO_Init();

    while(1) {

        unsigned sw1_pressed = !(GPIOC_IDR & SW1_IDR);
        unsigned sw2_pressed = !(GPIOC_IDR & SW2_IDR);

        
        if(sw1_pressed && sw2_pressed) {
            GPIOA_ODR |= PA5_ODR;
        } else if (sw1_pressed && !sw2_pressed) {
            GPIOA_ODR |= PA5_ODR;
            delay_ms(100);
            GPIOA_ODR &= ~PA5_ODR;
            delay_ms(100);
        } else if (!sw1_pressed && sw2_pressed) {
            GPIOA_ODR |= PA5_ODR;
            delay_ms(400);
            GPIOA_ODR &= ~PA5_ODR;
            delay_ms(400);
        } else {
            GPIOA_ODR &= ~PA5_ODR;
        }
    }
}














