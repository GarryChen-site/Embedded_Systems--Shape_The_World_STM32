

// =========================================================================
// RCC REGISTERS
// =========================================================================
#define RCC_AHB1ENR (*((volatile unsigned int *)0x40023830))
#define RCC_APB1ENR (*((volatile unsigned int *)0x40023840))

#define GPIOAEN (1 << 0)
#define UART2EN (1 << 17)

// =========================================================================
// GPIOA REGISTERS
// =========================================================================
#define GPIOA_MODER (*((volatile unsigned int *)0x40020000))
#define GPIOA_AFRL (*((volatile unsigned int *)0x40020020))

// =========================================================================
// UART2 REGISTERS
// =========================================================================
#define USART2_SR (*((volatile unsigned int *)0x40004400))
#define USART2_DR (*((volatile unsigned int *)0x40004404))
#define USART2_BRR (*((volatile unsigned int *)0x40004408))
#define USART2_CR1 (*((volatile unsigned int *)0x4000440C))

// SR bits
#define USART_SR_TC (1 << 6)  // Transmit complete
#define USART_SR_TXE (1 << 7) // Transmit data register empty

// CR1 bits
#define USART_CR1_TE (1 << 3)  // Transmitter enable
#define USART_CR1_UE (1 << 13) // USART enable