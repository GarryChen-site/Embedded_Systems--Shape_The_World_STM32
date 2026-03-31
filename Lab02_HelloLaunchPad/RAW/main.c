
#define RCC_AHB1ENR     (*((volatile unsigned int *)0x40023830))

#define GPIOA_MODER     (*((volatile unsigned int *)0x40020000))
#define GPIOA_ODR       (*((volatile unsigned int *)0x40020014))

#define GPIOC_MODER     (*((volatile unsigned int *)0x40020800))
#define GPIOC_IDR       (*((volatile unsigned int *)0x40020810))

void delay_ms(volatile unsigned int ms) {
    volatile unsigned int count = ms * 1600; 
    while (count > 0) {
        count--;
    }
}

int main(void) {
    RCC_AHB1ENR |= (1 << 0) | (1 << 2);

    GPIOA_MODER &= ~(3 << 10); 
    GPIOA_MODER |= (1 << 10);  

    GPIOC_MODER &= ~(3 << 26); 

    while (1) {
        GPIOA_ODR |= (1 << 5);     
        delay_ms(100);

        GPIOA_ODR &= ~(1 << 5);    

        if ((GPIOC_IDR & (1 << 13)) == 0) {
            delay_ms(400);
        } else {
            delay_ms(100);
        }
    }
    
    return 0;
}