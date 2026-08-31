#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* =========================================================================
 * PIN DEFINITIONS
 * ========================================================================= */

/* --- Traffic light outputs --- */
#define EW_RED_Pin GPIO_PIN_5 /* PB5  D4  */
#define EW_RED_Port GPIOB
#define EW_YELLOW_Pin GPIO_PIN_4 /* PB4  D5  */
#define EW_YELLOW_Port GPIOB
#define EW_GREEN_Pin GPIO_PIN_10 /* PB10 D6  */
#define EW_GREEN_Port GPIOB
#define NS_RED_Pin GPIO_PIN_8 /* PA8  D7  */
#define NS_RED_Port GPIOA
#define NS_YELLOW_Pin GPIO_PIN_9 /* PA9  D8  */
#define NS_YELLOW_Port GPIOA
#define NS_GREEN_Pin GPIO_PIN_7 /* PC7  D9  */
#define NS_GREEN_Port GPIOC

/* --- Walk light outputs --- */
#define WALK_Pin GPIO_PIN_6 /* PB6  D10  green LED */
#define WALK_Port GPIOB
#define DONTWALK_Pin GPIO_PIN_7 /* PA7  D11  red LED   */
#define DONTWALK_Port GPIOA

/* --- Sensor inputs (active HIGH: 10kΩ pull-down, switch to 3.3V) --- */
#define EW_SENSOR_Pin GPIO_PIN_6 /* PA6  D12 */
#define EW_SENSOR_Port GPIOA
#define NS_SENSOR_Pin GPIO_PIN_10 /* PA10 D2  */
#define NS_SENSOR_Port GPIOA
#define PED_SENSOR_Pin GPIO_PIN_3 /* PB3  D3  */
#define PED_SENSOR_Port GPIOB

/* =========================================================================
 * STATE IDENTIFIERS
 * One name per FSM state — used in the next[] arrays instead of raw numbers.
 * The enum values 0-10 match the FSM[] array indices exactly.
 * ========================================================================= */
typedef enum {
  S_EW_GREEN = 0,  /* EW green,  NS red    — east/west cars move          */
  S_EW_YELLOW = 1, /* EW yellow, NS red    — EW transitioning to red      */
  S_NS_GREEN = 2,  /* NS green,  EW red    — north/south cars move         */
  S_NS_YELLOW = 3, /* NS yellow, EW red    — NS transitioning to red      */
  S_WALK = 4,      /* Both red,  WALK ON   — pedestrians crossing          */
  S_FLASH1 = 5,    /* Both red,  all OFF   — "hurry up" flash 1 (50ms)    */
  S_FLASH2 = 6,    /* Both red,  DW ON     — "hurry up" flash 2 (50ms)    */
  S_FLASH3 = 7,    /* Both red,  all OFF   — "hurry up" flash 3 (50ms)    */
  S_FLASH4 = 8,    /* Both red,  DW ON     — "hurry up" flash 4 (50ms)    */
  S_FLASH5 = 9,    /* Both red,  all OFF   — "hurry up" flash 5 (50ms)    */
  S_FLASH6 = 10,   /* Both red,  DW ON     — "hurry up" flash 6 (50ms)    */
} StateId;

/* =========================================================================
 * FSM TYPE  — identical struct to original TableTrafficLight.c
 * ========================================================================= */
typedef struct {
  uint32_t time_ms; /* state duration in ms  (original: Time × 10ms)      */
  uint8_t traffic;  /* traffic bitmask, same bit layout as original PB[5:0] */
  uint8_t walk;     /* walk bitmask,    same bit layout as original PF[3,1] */
  StateId next[8];  /* next state indexed by sensor reading [0..7]          */
} State_t;

/* =========================================================================
 * FSM TABLE
 *
 * Sensor index (bit2=PED, bit1=NS_CAR, bit0=EW_CAR):
 *   next[0b000] = nobody    next[0b001] = EW only
 *   next[0b010] = NS only   next[0b011] = EW + NS
 *   next[0b100] = PED only  next[0b101] = PED + EW
 *   next[0b110] = PED + NS  next[0b111] = all three
 * ========================================================================= */

/*
 * 3 states(EW greem, NS green, Walk) + 2 yellow states + 6 flash states = 11
 * states Green phase time = 200ms Yellow phase time = 300ms Walk phase time =
 * 200ms Flash phase time = 50ms 3 Flashes need 6 States (On, Off, On, Off, On,
 * Off)
 */

static const State_t FSM[11] = {
    //  bit5=0 EW_RED  OFF    bit4=0 EW_YELLOW OFF    bit3=1 EW_GREEN  ON ✓
    //  bit2=1 NS_RED  ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN  OFF
    // -------------------------------------------------------------------
    // nobody -> stay EW green; EW only -> stay EW green; NS only -> go EW
    // yellow; EW+NS -> go EW yellow;
    // PED only -> go EW yellow; PED+EW -> go EW yellow; PED+NS -> go EW yellow;
    // all three -> go EW yellow
    {200,
     0x0C,
     0x02,
     {S_EW_GREEN, S_EW_GREEN, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW,
      S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW}}, /* S0: EW green  */

    // bit5=0 EW_RED OFF    bit4=1 EW_YELLOW ON ✓    bit3=0 EW_GREEN OFF
    // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF      bit0=0 NS_GREEN OFF
    // -------------------------------------------------------------------
    // nobody -> stay EW yellow; EW only -> go EW green; NS only -> go NS green;
    // EW+NS -> go NS green;
    // PED only -> go WALK; PED+EW -> go WALK; PED+NS -> go NS green; all three
    // -> go NS green
    {300,
     0x14,
     0x02,
     {S_EW_YELLOW, S_EW_GREEN, S_NS_GREEN, S_NS_GREEN, S_WALK, S_WALK,
      S_NS_GREEN, S_NS_GREEN}}, /* S1: EW yellow */

    // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
    // bit2=0 NS_RED OFF    bit1=0 NS_YELLOW OFF    bit0=1 NS_GREEN ON ✓
    // -------------------------------------------------------------------
    // nobody -> stay NS green; EW only -> go NS yellow; NS only -> stay NS
    // green; EW+NS -> go NS yellow;
    // PED only -> go NS yellow; PED+EW -> go NS yellow; PED+NS -> go NS yellow;
    // all three -> go NS yellow
    {200,
     0x21,
     0x02,
     {S_NS_GREEN, S_NS_YELLOW, S_NS_GREEN, S_NS_YELLOW, S_NS_YELLOW,
      S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW}}, /* S2: NS green  */

    // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
    // bit2=0 NS_RED OFF    bit1=1 NS_YELLOW ON ✓   bit0=0 NS_GREEN OFF
    // -------------------------------------------------------------------
    // nobody -> stay NS yellow; EW only -> go EW green; NS only -> go NS green;
    // EW+NS -> go EW green;
    // PED only -> go WALK; PED+EW -> go EW green; PED+NS -> go WALK; all three
    // -> go WALK
    {300,
     0x22,
     0x02,
     {S_NS_YELLOW, S_EW_GREEN, S_NS_GREEN, S_EW_GREEN, S_WALK, S_EW_GREEN,
      S_WALK, S_WALK}}, /* S3: NS yellow */

    // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
    // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN OFF
    // -------------------------------------------------------------------
    // a car arriving is the only trigger to end the WALK state.
    // nobody -> stay WALK; EW only -> go FLASH 1; NS only -> go FLASH 1; EW+NS
    // -> go FLASH 1;
    // PED only -> stay WALK; PED+EW -> go FLASH 1; PED+NS -> go FLASH 1; all
    // three -> go FLASH 1
    {200,
     0x24,
     0x08,
     {S_WALK, S_FLASH1, S_FLASH1, S_FLASH1, S_WALK, S_FLASH1, S_FLASH1,
      S_FLASH1}}, /* S4: WALK      */

    // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
    // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN OFF
    // -------------------------------------------------------------------
    // during the flash chain, the only escape back to WALK is when no car is
    // detected. Any car keeps the flash advancing forward
    // nobody -> go WALK; EW only -> go FLASH 2; NS only -> go FLASH 2; EW+NS ->
    // go FLASH 2;
    // PED only -> go WALK; PED+EW -> go FLASH 2; PED+NS -> go FLASH 2; all
    // three -> go FLASH 2
    {50,
     0x24,
     0x00,
     {S_WALK, S_FLASH2, S_FLASH2, S_FLASH2, S_WALK, S_FLASH2, S_FLASH2,
      S_FLASH2}}, /* S5: flash 1   */
    {50,
     0x24,
     0x02,
     {S_WALK, S_FLASH3, S_FLASH3, S_FLASH3, S_WALK, S_FLASH3, S_FLASH3,
      S_FLASH3}}, /* S6: flash 2   */
    {50,
     0x24,
     0x00,
     {S_WALK, S_FLASH4, S_FLASH4, S_FLASH4, S_WALK, S_FLASH4, S_FLASH4,
      S_FLASH4}}, /* S7: flash 3   */
    {50,
     0x24,
     0x02,
     {S_WALK, S_FLASH5, S_FLASH5, S_FLASH5, S_WALK, S_FLASH5, S_FLASH5,
      S_FLASH5}}, /* S8: flash 4   */
    {50,
     0x24,
     0x00,
     {S_WALK, S_FLASH6, S_FLASH6, S_FLASH6, S_WALK, S_FLASH6, S_FLASH6,
      S_FLASH6}}, /* S9: flash 5   */

    // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
    // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN OFF
    // -------------------------------------------------------------------
    // a car must be waiting to break out of the walk/flash cycle.
    // If no cars: with pedestrian → WALK again; without pedestrian → restart
    // the flash loop at FLASH1.
    // nobody -> restart FLASH 1; EW only -> go EW GREEN; NS only -> go NS
    // GREEN; EW+NS -> go EW GREEN;
    // PED only -> go WALK; PED+EW -> go EW GREEN; PED+NS -> go NS GREEN; all
    // three -> go EW GREEN
    {50,
     0x24,
     0x02,
     {S_FLASH1, S_EW_GREEN, S_NS_GREEN, S_EW_GREEN, S_WALK, S_EW_GREEN,
      S_NS_GREEN, S_EW_GREEN}}, /* S10: flash 6  */
};
static void apply_traffic(uint8_t traffic, uint8_t walk);
static uint8_t read_sensors(void);

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t state = 0; /* FSM starts in state 0: EW green             */

  /* ---- LED WIRING TEST: each LED on for 300ms in sequence ---- */
  GPIO_TypeDef *led_ports[] = {EW_RED_PORT, EW_YELLOW_PORT, EW_GREEN_PORT,
                               NS_RED_PORT, NS_YELLOW_PORT, NS_GREEN_PORT,
                               WALK_PORT,   DONTWALK_PORT};
  uint16_t led_pins[] = {EW_RED_PIN, EW_YELLOW_PIN, EW_GREEN_PIN,
                         NS_RED_PIN, NS_YELLOW_PIN, NS_GREEN_PIN,
                         WALK_PIN,   DONTWALK_PIN};
  for (int k = 0; k < 8; k++) {
    HAL_GPIO_WritePin(led_ports[k], led_pins[k], GPIO_PIN_SET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(led_ports[k], led_pins[k], GPIO_PIN_RESET);
    HAL_Delay(100);
  }
  /* ---- END LED TEST ---- */

  /* ---- SWITCH TEST: press each switch, its LED lights ---- */
  /* EW_SENSOR  → lights EW_GREEN  */
  /* NS_SENSOR  → lights NS_GREEN  */
  /* PED_SENSOR → lights WALK      */
  uint32_t test_end = HAL_GetTick() + 10000; /* 10 second test window */
  while (HAL_GetTick() < test_end) {
    HAL_GPIO_WritePin(EW_GREEN_PORT, EW_GREEN_PIN,
                      HAL_GPIO_ReadPin(EW_SENSOR_PORT, EW_SENSOR_PIN));
    HAL_GPIO_WritePin(NS_GREEN_PORT, NS_GREEN_PIN,
                      HAL_GPIO_ReadPin(NS_SENSOR_PORT, NS_SENSOR_PIN));
    HAL_GPIO_WritePin(WALK_PORT, WALK_PIN,
                      HAL_GPIO_ReadPin(PED_SENSOR_PORT, PED_SENSOR_PIN));
  }
  /* ---- END SWITCH TEST ---- */

  while (1) {
    apply_traffic(FSM[state].traffic, FSM[state].walk); /* set LEDs */
    HAL_Delay(FSM[state].time_ms);                      /* wait     */
    state = FSM[state].next[read_sensors()];            /* advance  */
  }
  /* USER CODE END 3 */
}

/* =========================================================================
 * apply_traffic
 * Write the Traffic and Walk bitmasks to the physical GPIO pins.
 *
 * Traffic bitmask (identical encoding to original PB[5:0]):
 *   bit5=EW_RED  bit4=EW_YELLOW  bit3=EW_GREEN
 *   bit2=NS_RED  bit1=NS_YELLOW  bit0=NS_GREEN
 *
 * Walk bitmask (identical encoding to original PF[3,1]):
 *   bit3=WALK  bit1=DONTWALK
 * ========================================================================= */
static void apply_traffic(uint8_t traffic, uint8_t walk) {
  HAL_GPIO_WritePin(GPIOB, EW_RED_Pin, (GPIO_PinState)((traffic >> 5) & 1));
  HAL_GPIO_WritePin(GPIOB, EW_YELLOW_Pin, (GPIO_PinState)((traffic >> 4) & 1));
  HAL_GPIO_WritePin(GPIOB, EW_GREEN_Pin, (GPIO_PinState)((traffic >> 3) & 1));
  HAL_GPIO_WritePin(GPIOA, NS_RED_Pin, (GPIO_PinState)((traffic >> 2) & 1));
  HAL_GPIO_WritePin(GPIOA, NS_YELLOW_Pin, (GPIO_PinState)((traffic >> 1) & 1));
  HAL_GPIO_WritePin(GPIOC, NS_GREEN_Pin, (GPIO_PinState)((traffic >> 0) & 1));

  HAL_GPIO_WritePin(GPIOB, WALK_Pin, (GPIO_PinState)((walk >> 3) & 1));
  HAL_GPIO_WritePin(GPIOA, DONTWALK_Pin, (GPIO_PinState)((walk >> 1) & 1));
}

/* =========================================================================
 * read_sensors
 * Read the three sensor inputs and compose a 3-bit index.
 *
 * Returns: bit2=PED  bit1=NS_CAR  bit0=EW_CAR  (same as original PE[2:0])
 * Active HIGH: switch pressed → sensor = 1
 * ========================================================================= */
static uint8_t read_sensors(void) {
  uint8_t s = 0;
  s |= (uint8_t)HAL_GPIO_ReadPin(GPIOA, EW_SENSOR_Pin) << 0;
  s |= (uint8_t)HAL_GPIO_ReadPin(GPIOA, NS_SENSOR_Pin) << 1;
  s |= (uint8_t)HAL_GPIO_ReadPin(GPIOB, PED_SENSOR_Pin) << 2;
  return s; /* 0..7 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin | DONTWALK_Pin | NS_RED_Pin | NS_YELLOW_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, EW_GREEN_Pin | EW_YELLOW_Pin | EW_RED_Pin | WALK_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(NS_GREEN_GPIO_Port, NS_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin | USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin DONTWALK_Pin NS_RED_Pin NS_YELLOW_Pin */
  GPIO_InitStruct.Pin = LD2_Pin | DONTWALK_Pin | NS_RED_Pin | NS_YELLOW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : EW_SENSOR_Pin NS_SENSOR_Pin */
  GPIO_InitStruct.Pin = EW_SENSOR_Pin | NS_SENSOR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : EW_GREEN_Pin EW_YELLOW_Pin EW_RED_Pin WALK_Pin */
  GPIO_InitStruct.Pin = EW_GREEN_Pin | EW_YELLOW_Pin | EW_RED_Pin | WALK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : NS_GREEN_Pin */
  GPIO_InitStruct.Pin = NS_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(NS_GREEN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PED_SENSOR_Pin */
  GPIO_InitStruct.Pin = PED_SENSOR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PED_SENSOR_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
