
// ============================================================
// Register definitions
// ============================================================

// RCC
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U)) // §6.3.9
#define RCC_APB2ENR (*((volatile uint32_t *)0x40023844U)) // §6.3.15

// GPIOA
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_AFRH (*((volatile uint32_t *)0x40020024U)) // pins 8-15

// USART1
#define USART1_SR (*((volatile uint32_t *)0x40011000U))  // status
#define USART1_DR (*((volatile uint32_t *)0x40011004U))  // data
#define USART1_BRR (*((volatile uint32_t *)0x40011008U)) // baud
#define USART1_CR1 (*((volatile uint32_t *)0x4001100CU)) // control 1

// USART1 SR flag bits
#define USART_SR_TXE (1U << 7)  // transmit data reg empty
#define USART_SR_RXNE (1U << 5) // read data reg not empty

// USART1 CR1 control bits
#define USART_CR1_UE (1U << 13) // USART enable
#define USART_CR1_TE (1U << 3)  // transmitter enable
#define USART_CR1_RE (1U << 2)  // receiver enable