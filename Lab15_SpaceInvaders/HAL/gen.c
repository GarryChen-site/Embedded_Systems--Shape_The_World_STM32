/* gen.c — Lab 15 Space Invaders, STM32 F446RE (HAL / CubeIDE version)
 *
 * This file shows the complete content of the CubeIDE-generated main.c with
 * all USER CODE blocks filled in.  Copy the content inside each USER CODE
 * section into the corresponding section of your CubeIDE main.c.
 *
 * HARDWARE:
 *   Trimmer pot wiper  → PA0  (ADC1 CH0, CN8 pin A0)
 *   OLED GND           → GND
 *   OLED VCC           → 3.3V (CN8 pin 4)
 *   OLED SCL           → PB6  (I2C1_SCL default, CN10 pin 17)
 *                         or PB8 if remapped (CN10 pin 3 / Arduino D15)
 *   OLED SDA           → PB7  (I2C1_SDA default, CN10 pin 19)
 *                         or PB9 if remapped (CN10 pin 5 / Arduino D14)
 *   R-2R DAC PB0(LSB)  → Buzzer+  (CN8 pin 5 / Arduino A3)
 *   R-2R DAC PB1       → R-2R ladder node
 *   R-2R DAC PB2       → R-2R ladder node
 *   R-2R DAC PB3(MSB)  → R-2R ladder node  (CN10 pin 26 / Arduino D3)
 *   Fire button 1      → PC6  (CN10 pin 13, pull-down, EXTI6 rising)
 *   Fire button 2      → PC8  (CN10 pin 3,  pull-down, EXTI8 rising)
 *
 * IOC SETTINGS (see README.md for full step-by-step guide):
 *   Analog      → ADC1:  IN0 single-ended, 12-bit, 480 cycles
 *   Connectivity → I2C1: Fast Mode 400 kHz (PB6=SCL, PB7=SDA default)
 *   GPIO:
 *     PB0, PB1, PB2, PB3 → GPIO_Output (push-pull, no pull, low speed)
 *     PC6, PC8            → GPIO_EXTI (rising edge, pull-down)
 *   NVIC: EXTI9_5 = enabled
 *
 * FILES NEEDED IN Core/Src and Core/Inc (copy from HAL/ directory):
 *   ssd1306_hal.c / ssd1306_hal.h   — OLED driver
 *   dac_hal.c     / dac_hal.h       — R-2R DAC driver
 *   sound_hal.c   / sound_hal.h     — Sound driver
 *   game_types.h                    — STyp struct (shared)
 *   random.c      / random.h        — PRNG (copy from RAW/)
 *   SpaceInvaders.c                 — game logic (see README for edits)
 */

/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "ssd1306_hal.h"
#include "dac_hal.h"
#include "sound_hal.h"
#include "random.h"
#include "game_types.h"   /* STyp — shared with SpaceInvaders.c */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
/* ── extern game globals defined in SpaceInvaders.c ── */
extern volatile uint32_t ADCdata;
extern volatile uint32_t playerPosition;
extern volatile uint32_t playerFlag;
extern volatile uint32_t missileFlag;
extern volatile uint32_t laserFlag;
extern          uint32_t randomnum;
extern          uint32_t currentLevel;
extern          int32_t  enemyDir;

/* Missile/Laser arrays for EXTI button callback */
extern STyp Missile[9];   /* [8] = player missile */
extern STyp Laser[9];     /* [8] = player laser   */
extern uint32_t playMissile;
extern uint32_t playLaser;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
/* ── extern game functions defined in SpaceInvaders.c ── */
extern void Enemy_Init(void);
extern void Bunker_Init(void);
extern void Player_Init(void);
extern void Draw(void);
extern void Enemy_Move(void);
extern void UFO_Move(void);
extern void Enemy_Missile_Init(void);
extern void Enemy_Missile_Move(void);
extern void Enemy_Laser_Init(void);
extern void Enemy_Laser_Move(void);
extern void Player_Move(uint32_t pos);
extern void Player_Missile_Init(void);
extern void Player_Laser_Init(void);
extern void Missile_Move(void);
extern void Laser_Move(void);
extern void Level_Check(void);

/* Local helpers */
static uint32_t ADC_Read(void);
static uint32_t Convert(uint32_t sample);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ADC_Read — trigger one software conversion and return 12-bit result */
static uint32_t ADC_Read(void) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 5U) == HAL_OK)
        return HAL_ADC_GetValue(&hadc1);
    return 2048U;   /* mid-scale fallback on timeout */
}

/* Convert — ADC 12-bit sample → player X position (0 to SCREENW-PLAYERW)
 * Original TM4C: (66 × sample) / 4095  where 66 = 84 - PLAYERW(18)
 * STM32 128px:   (110 × sample) / 4095  where 110 = 128 - PLAYERW(18)  */
static uint32_t Convert(uint32_t sample) {
    return (110U * sample) / 4095U;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* ─── CubeIDE-generated init ─── */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();     /* configures PB0-PB3 outputs, PC6/PC8 EXTI inputs */
    MX_ADC1_Init();     /* ADC1 CH0 PA0, 12-bit, 480 cycles */
    MX_I2C1_Init();     /* I2C1 400 kHz, PB6=SCL PB7=SDA */

    /* USER CODE BEGIN 2 */
    Sound_HAL_Init();          /* zero R-2R DAC outputs            */
    SSD1306_Init();            /* send SSD1306 startup sequence     */
    Random_Init(1U);           /* seed XorShift32 PRNG             */

    currentLevel = 1U;
    enemyDir     = 1;
    Enemy_Init();
    Bunker_Init();
    Player_Init();

    SSD1306_ClearBuffer();
    SSD1306_DisplayBuffer();
    Draw();
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    uint32_t lastTick = HAL_GetTick();

    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        /* ─── 30 Hz ADC sample (every 33 ms via HAL_GetTick) ─── */
        uint32_t now = HAL_GetTick();
        if ((now - lastTick) >= 33U) {
            lastTick      = now;
            ADCdata       = ADC_Read();
            playerPosition = Convert(ADCdata);
            playerFlag    = 1U;
        }

        /* ─── Game loop (runs every iteration; heavy ops gated on flags) ─── */
        randomnum = (Random32() >> 24U) % 60U;

        Enemy_Missile_Init();
        Enemy_Laser_Init();
        Enemy_Missile_Move();
        Enemy_Laser_Move();
        Level_Check();

        if (playerFlag) {
            Player_Move(playerPosition);
            playerFlag = 0U;
        }
        if (missileFlag) {
            Player_Missile_Init();
            missileFlag = 0U;
        }
        if (laserFlag) {
            Player_Laser_Init();
            laserFlag = 0U;
        }

        if (Missile[playMissile].life > 0) Missile_Move();
        if (Laser[playLaser].life   > 0) Laser_Move();

        Enemy_Move();
        UFO_Move();
        Draw();

        HAL_Delay(5U);   /* brief yield; I2C DisplayBuffer adds ~23 ms */
    }
    /* USER CODE END 3 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief  EXTI GPIO callback — fire buttons PC6 (missile) and PC8 (laser).
  *         CubeIDE calls this from EXTI9_5_IRQHandler via HAL_GPIO_EXTI_IRQHandler.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_6) {          /* PC6 → missile */
        if (Missile[playMissile].life <= 0) {
            missileFlag = 1U;
            randomnum  += 3U;
        }
    }
    if (GPIO_Pin == GPIO_PIN_8) {          /* PC8 → laser */
        if (Laser[playLaser].life <= 0) {
            laserFlag = 1U;
            randomnum += 5U;
        }
    }
}
/* USER CODE END 4 */

/* ── CubeIDE-generated peripheral init stubs (filled by IOC) ────────────── */

void SystemClock_Config(void)
{
    /* HSI 16 MHz, no PLL, APB1=APB2=HCLK=16 MHz */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1| RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance              = I2C1;
    hi2c1.Init.ClockSpeed       = 400000;
    hi2c1.Init.DutyCycle        = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PA0 — ADC1 IN0 (configured by MX_ADC1_Init via HAL_ADC_MspInit) */

    /* PB0, PB1, PB2, PB4 — R-2R DAC outputs (push-pull, no pull)
     * NOTE: PB3 is SWO debug trace on Nucleo-F446RE — do NOT configure it as GPIO. */
    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PC6, PC8 — fire buttons, EXTI rising edge, external pull-down to GND */
    GPIO_InitStruct.Pin   = GPIO_PIN_6 | GPIO_PIN_8;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Enable EXTI9_5 interrupt in NVIC (covers EXTI6 and EXTI8) */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}
