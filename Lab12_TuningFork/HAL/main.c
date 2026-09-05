

/* ─── USER CODE BEGIN PV ────────────────────────────────────────────────────
 * Private variables — paste into CubeIDE's USER CODE BEGIN PV region
 * ─────────────────────────────────────────────────────────────────────────*/
volatile uint32_t forkOn = 0;   /* 1 = buzzer running, 0 = silent        */
volatile uint32_t input_sw = 0; /* state machine: 0-1-2-3-0              */
/* ─── USER CODE END PV ──────────────────────────────────────────────────────*/

/* ─── USER CODE BEGIN 2 ─────────────────────────────────────────────────────
 * Inside main(), after MX_TIM2_Init() — start the 880 Hz interrupt
 * ─────────────────────────────────────────────────────────────────────────*/
HAL_TIM_Base_Start_IT(&htim2);
/* ─── USER CODE END 2 ───────────────────────────────────────────────────────*/

/* ─── USER CODE BEGIN 4 ─────────────────────────────────────────────────────
 * TIM2 overflow callback — fires at 880 Hz, drives 440 Hz square wave.
 * Replaces SysTick_Handler from the bare-metal version.
 * Paste this entire function into CubeIDE's USER CODE BEGIN 4 region.
 * ─────────────────────────────────────────────────────────────────────────*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance != TIM2)
    return; /* guard: ignore other timers */

  /* PC13 is active LOW: GPIO_PIN_RESET means button is pressed */
  int sw =
      (HAL_GPIO_ReadPin(SW_B1_GPIO_Port, SW_B1_Pin) == GPIO_PIN_RESET) ? 1 : 0;

  /* ── 4-state toggle machine (identical to bare-metal SysTick_Handler) ── */
  if (sw && !input_sw) {
    forkOn = 1;
    input_sw = 1;
  } /* press #1  → ON  */
  else if (sw && input_sw == 2) {
    forkOn = 0;
    input_sw = 3;
  } /* press #2  → OFF */
  else if (!sw && input_sw == 1) {
    forkOn = 1;
    input_sw = 2;
  } /* release#1 → ON  */
  else if (!sw && input_sw == 3) {
    forkOn = 0;
    input_sw = 0;
  } /* release#2 → OFF */

  /* ── Drive buzzer on PA6 ── */
  if (forkOn) {
    HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin); /* square wave */
  } else {
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin,
                      GPIO_PIN_RESET); /* silent */
  }
}
/* ─── USER CODE END 4 ───────────────────────────────────────────────────────*/
