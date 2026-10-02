/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "esp32_hal.h"
#include "ssd1306_hal.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// =========================================================================
// Wi-Fi Configuration
// Replace with your local Wi-Fi Access Point credentials
// =========================================================================
#define WIFI_SSID "Your_SSID"
#define WIFI_PASS "Your_Password"

#define QUERY_INTERVAL_SEC 60
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static char ipAddr[32] = {0};
static char city[32] = {0};
static char temp[16] = {0};
static char desc[48] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static int Button_IsPressed(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static int Button_IsPressed(void) {
  // Blue User Button B1 on PC13 is active LOW
  return (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET);
}
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
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  // 1. Initialize ESP32 Driver Handles and OLED
  ESP32_HAL_Init(&huart1, &huart2);
  SSD1306_Init();

  // 2. Greeting on Console & OLED
  UART_PC_Println("\r\n==========================================");
  UART_PC_Println("  Lab 16: IoT Weather Station (STM32 HAL) ");
  UART_PC_Println("==========================================");

  SSD1306_Clear();
  SSD1306_SetCursor(0, 0);
  SSD1306_OutString("Lab 16: IoT Weather");
  SSD1306_SetCursor(0, 2);
  SSD1306_OutString("Connecting ESP32...");
  SSD1306_DisplayBuffer();

  UART_PC_Println("[INIT] Connecting to ESP32 coprocessor...");

  // 3. Test ESP32 Link (USART1)
  if (ESP32_HAL_Ping(3000) != ESP32_OK) {
    UART_PC_Println("[ERROR] ESP32 not responding! Check PA9/PA10 wiring.");
    SSD1306_SetCursor(0, 4);
    SSD1306_OutString("ESP32: NOT FOUND!");
    SSD1306_SetCursor(0, 6);
    SSD1306_OutString("Check PA9/PA10 wires");
    SSD1306_DisplayBuffer();
    while (1) {
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
      HAL_Delay(500);
    }
  }

  UART_PC_Println("[INIT] ESP32 ping OK!");
  SSD1306_SetCursor(0, 3);
  SSD1306_OutString("ESP32: Connected");
  SSD1306_SetCursor(0, 5);
  SSD1306_OutString("Connecting Wi-Fi...");
  SSD1306_DisplayBuffer();

  // 4. Connect to Wi-Fi
  UART_PC_Print("[WIFI] Connecting to SSID: ");
  UART_PC_Print(WIFI_SSID);
  UART_PC_Println(" ...");

  if (ESP32_HAL_ConnectWiFi(WIFI_SSID, WIFI_PASS, ipAddr, sizeof(ipAddr),
                            20000) != ESP32_OK) {
    UART_PC_Println("[ERROR] Wi-Fi connection failed! Update SSID/PASS.");
    SSD1306_Clear();
    SSD1306_SetCursor(0, 1);
    SSD1306_OutString("Wi-Fi Connection");
    SSD1306_SetCursor(0, 3);
    SSD1306_OutString("FAILED!");
    SSD1306_SetCursor(0, 5);
    SSD1306_OutString("Check SSID/PASS");
    SSD1306_DisplayBuffer();
    while (1) {
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
      HAL_Delay(250);
    }
  }

  UART_PC_Print("[WIFI] Connected successfully! IP: ");
  UART_PC_Println(ipAddr);
  UART_PC_Println("");

  SSD1306_Clear();
  SSD1306_SetCursor(0, 0);
  SSD1306_OutString("Wi-Fi Connected!");
  SSD1306_SetCursor(0, 2);
  SSD1306_OutString("IP: ");
  SSD1306_OutString(ipAddr);
  SSD1306_SetCursor(0, 5);
  SSD1306_OutString("Fetching weather...");
  SSD1306_DisplayBuffer();

  HAL_Delay(1500);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    UART_PC_Println("------------------------------------------");
    UART_PC_Println("[QUERY] Requesting current weather from ESP32...");

    SSD1306_Clear();
    SSD1306_SetCursor(0, 0);
    SSD1306_OutString("=== IOT WEATHER ===");
    SSD1306_SetCursor(0, 3);
    SSD1306_OutString("Fetching data...");
    SSD1306_DisplayBuffer();

    // Indicate network activity with LD2 (PA5)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    int res = ESP32_HAL_GetWeather(NULL, city, temp, desc, 12000);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    if (res == ESP32_OK) {
      // Log to PC Console
      UART_PC_Println("[SUCCESS] Weather report received:");
      UART_PC_Print("  Location:    ");
      UART_PC_Println(city);
      UART_PC_Print("  Temperature: ");
      UART_PC_Print(temp);
      UART_PC_Println(" C");
      UART_PC_Print("  Condition:   ");
      UART_PC_Println(desc);

      // Display on SSD1306 OLED
      SSD1306_Clear();
      SSD1306_SetCursor(0, 0);
      SSD1306_OutString("=== IOT WEATHER ===");

      SSD1306_SetCursor(0, 2);
      SSD1306_OutString("City: ");
      SSD1306_OutString(city);

      SSD1306_SetCursor(0, 4);
      SSD1306_OutString("Temp: ");
      SSD1306_OutString(temp);
      SSD1306_OutString(" C");

      SSD1306_SetCursor(0, 6);
      SSD1306_OutString(desc);

      SSD1306_DisplayBuffer();
    } else {
      UART_PC_Println("[ERROR] Weather query failed or timed out!");
      SSD1306_Clear();
      SSD1306_SetCursor(0, 1);
      SSD1306_OutString("Weather Query Error");
      SSD1306_SetCursor(0, 3);
      SSD1306_OutString("Check ESP32 / WiFi");
      SSD1306_SetCursor(0, 5);
      SSD1306_OutString("Press B1 to retry");
      SSD1306_DisplayBuffer();
    }

    // Wait 60 seconds before next auto-query, or refresh immediately on B1
    // (PC13) button press
    UART_PC_Println("[INFO] Press PC13 blue button to refresh immediately.");
    for (int i = 0; i < (QUERY_INTERVAL_SEC * 10); i++) { // 600 * 100ms = 60s
      if (Button_IsPressed()) {
        HAL_Delay(50); // Debounce
        if (Button_IsPressed()) {
          while (Button_IsPressed()) {
            HAL_Delay(10);
          }
          UART_PC_Println("[USER] Button pressed! Refreshing weather data...");
          break;
        }
      }
      HAL_Delay(100);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
