#include <stdint.h>
#include "uart.h"

#define RCC_AHB1ENR     (*((volatile unsigned int *)0x40023830))
#define RCC_APB1ENR     (*((volatile unsigned int *)0x40023840))

#define GPIOAEN         (1 << 0)
#define UART2EN         (1 << 17)



#define GPIOA_MODER     (*((volatile unsigned int *)0x40020000))
#define GPIOA_OSPEEDR   (*((volatile unsigned int *)0x40020008))
#define GPIOA_AFRL      (*((volatile unsigned int *)0x40020020))


#define PA2_MODER_MASK  (3 << 4)
#define PA2_MODER_AF    (2 << 4)
#define PA3_MODER_MASK  (3 << 6)
#define PA3_MODER_AF    (2 << 6)

#define PA2_SPEED_MASK  (3 << 4)
#define PA2_SPEED_HIGH  (3 << 4)
#define PA3_SPEED_MASK  (3 << 6)
#define PA3_SPEED_HIGH  (3 << 6)

#define PA2_AFRL_MASK   (0xF << 8)
#define PA2_AF7    (7 << 8)
#define PA3_AFRL_MASK   (0xF << 12)
#define PA3_AF7    (7 << 12)


#define USART2_SR       (*((volatile unsigned int *)0x40004400))
#define USART2_DR       (*((volatile unsigned int *)0x40004404))
#define USART2_BRR      (*((volatile unsigned int *)0x40004408))
#define USART2_CR1      (*((volatile unsigned int *)0x4000440C))


#define USART2_TXE      (1 << 7)
#define USART2_RXNE       (1 << 5)

#define USART_CR1_UE    (1 << 13)
#define USART_CR1_TE    (1 << 3)
#define USART_CR1_RE    (1 << 2)


void UART_Init(void) {
    // 1. Enable GPIOA and UART2 clocks
    RCC_AHB1ENR |= GPIOAEN;
    RCC_APB1ENR |= UART2EN;


    // 2. Configure PA2 and PA3 as alternate function (AF7)
    GPIOA_MODER &= ~(PA2_MODER_MASK | PA3_MODER_MASK);
    GPIOA_MODER |= (PA2_MODER_AF | PA3_MODER_AF);

    GPIOA_OSPEEDR &= ~(PA2_SPEED_MASK | PA3_SPEED_MASK);
    GPIOA_OSPEEDR |= (PA2_SPEED_HIGH | PA3_SPEED_HIGH);

    GPIOA_AFRL &= ~(PA2_AFRL_MASK | PA3_AFRL_MASK);
    GPIOA_AFRL |= (PA2_AF7 | PA3_AF7);

    // 3. Disable UART2 before configuration
    USART2_CR1 &= ~USART_CR1_UE;

    // 4. Set baud rate
    // STM32 (16 MHz HSI, APB1 = 16 MHz):
    //   OVER8 = 0 (16× oversampling, default)  RM0390 §27.3.4
    //   USARTDIV = fPCLK1 / (16 × baud)
    //            = 16,000,000 / (16 × 115,200)
    //            = 8.6806...
    //   DIV_Mantissa = 8    (integer part)
    //   DIV_Fraction = round(0.6806 × 16) = round(10.89) = 11 = 0xB
    //   BRR = (8 << 4) | 0xB = 0x8B = 139
    USART2_BRR = 0x8B;

    // 5. Enable UART2 (enable TX, RX and UART)
    USART2_CR1 |= (USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);
}


unsigned char UART_InChar(void) {
    while(!(USART2_SR & USART2_RXNE));
    return (unsigned char)USART2_DR;
}

void UART_OutChar(unsigned char data) {
    while(!(USART2_SR & USART2_TXE));
    USART2_DR = data;
}