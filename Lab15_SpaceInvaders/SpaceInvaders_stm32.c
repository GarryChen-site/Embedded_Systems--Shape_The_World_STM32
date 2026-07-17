

// RCC
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define RCC_APB1ENR (*((volatile uint32_t *)0x40023840U))
#define RCC_APB2ENR (*((volatile uint32_t *)0x40023844U))

// GPIOA (ADC input PA0)
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))

// GPIOC (fire buttons PC6, PC8)
#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

// ADC1
#define ADC1_SR (*((volatile uint32_t *)0x40012000U))
#define ADC1_CR1 (*((volatile uint32_t *)0x40012004U))
#define ADC1_CR2 (*((volatile uint32_t *)0x40012008U))
#define ADC1_SMPR2 (*((volatile uint32_t *)0x40012014U))
#define ADC1_SQR1 (*((volatile uint32_t *)0x4001202CU))
#define ADC1_SQR3 (*((volatile uint32_t *)0x40012034U))
#define ADC1_DR (*((volatile uint32_t *)0x4001204CU))
#define ADC_CCR (*((volatile uint32_t *)0x40012304U))

// SYSCFG (for EXTI routing)
#define SYSCFG_EXTICR2 (*((volatile uint32_t *)0x40013808U)) // EXTI4-7
#define SYSCFG_EXTICR3 (*((volatile uint32_t *)0x4001380CU)) // EXTI8-11

// EXTI
#define EXTI_IMR (*((volatile uint32_t *)0x40013C00U))
#define EXTI_RTSR (*((volatile uint32_t *)0x40013C08U))
#define EXTI_PR (*((volatile uint32_t *)0x40013C14U))

// NVIC
#define NVIC_ISER0 (*((volatile uint32_t *)0xE000E100U)) // IRQ0-31
#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20U))

// SysTick
#define STK_CTRL (*((volatile uint32_t *)0xE000E010U))
#define STK_LOAD (*((volatile uint32_t *)0xE000E014U))
#define STK_VAL (*((volatile uint32_t *)0xE000E018U))