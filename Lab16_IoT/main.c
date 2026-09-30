// main.c — Lab 16 IoT Weather Station (STM32 Bare-Metal Port)
//
// Hardware:
//   - STM32F446RE Nucleo-64
//   - NodeMCU ESP32 (Wi-Fi Coprocessor)
//   - SSD1306 0.96" OLED (128x64 I2C)
//
// Connections:
//   - USART2: PA2(TX), PA3(RX) -> ST-LINK USB to PC (115200 baud)
//   - USART1: PA9(TX), PA10(RX) -> ESP32 RX2(GPIO16), TX2(GPIO17)
//   - I2C1:   PB6(SCL), PB7(SDA) -> SSD1306 SCL, SDA
//   - Button: PC13 (Blue user button) -> Manual Weather Refresh
//   - LED:    PA5 (Green user LED)    -> Status indicator

#include "esp32.h"
#include "ssd1306.h"
#include "uart.h"
#include <stdint.h>
#include <string.h>

// =========================================================================
// Wi-Fi Configuration
// Replace with your local Wi-Fi Access Point credentials
// =========================================================================
#define WIFI_SSID "Your_SSID"
#define WIFI_PASS "Your_Password"

// =========================================================================
// Hardware Registers for LED (PA5) and Button (PC13)
// =========================================================================
#define RCC_AHB1ENR (*((volatile uint32_t *)0x40023830U))
#define GPIOA_MODER (*((volatile uint32_t *)0x40020000U))
#define GPIOA_ODR (*((volatile uint32_t *)0x40020014U))

#define GPIOC_MODER (*((volatile uint32_t *)0x40020800U))
#define GPIOC_PUPDR (*((volatile uint32_t *)0x4002080CU))
#define GPIOC_IDR (*((volatile uint32_t *)0x40020810U))

static void LED_Init(void) {
  RCC_AHB1ENR |= (1U << 0); // GPIOAEN
  GPIOA_MODER &= ~(3U << 10);
  GPIOA_MODER |= (1U << 10); // PA5 as Output
  GPIOA_ODR &= ~(1U << 5);   // Initially OFF
}

static void LED_On(void) { GPIOA_ODR |= (1U << 5); }
static void LED_Off(void) { GPIOA_ODR &= ~(1U << 5); }
static void LED_Toggle(void) { GPIOA_ODR ^= (1U << 5); }

static void Button_Init(void) {
  RCC_AHB1ENR |= (1U << 2);   // GPIOCEN
  GPIOC_MODER &= ~(3U << 26); // PC13 as Input
  GPIOC_PUPDR &= ~(3U << 26);
  GPIOC_PUPDR |= (1U << 26); // Pull-up
}

static int Button_IsPressed(void) {
  return ((GPIOC_IDR & (1U << 13)) == 0); // Active LOW on Nucleo
}

// =========================================================================
// Application Entry Point
// =========================================================================
int main(void) {
  char ipAddr[32] = {0};
  char city[32] = {0};
  char temp[16] = {0};
  char desc[48] = {0};

  // 1. Initialize Peripherals
  LED_Init();
  Button_Init();
  UART2_Init();   // PC Console 115200 baud
  SSD1306_Init(); // 128x64 OLED

  // 2. Greeting on Console & OLED
  UART2_OutString("\r\n==========================================\r\n");
  UART2_OutString("    Lab 16: IoT Weather Station (STM32)   \r\n");
  UART2_OutString("==========================================\r\n");

  SSD1306_Clear();
  SSD1306_SetCursor(0, 0);
  SSD1306_OutString("Lab 16: IoT Weather");
  SSD1306_SetCursor(0, 2);
  SSD1306_OutString("Connecting ESP32...");
  SSD1306_DisplayBuffer();

  UART2_OutString("[INIT] Connecting to ESP32 coprocessor...\r\n");

  // 3. Initialize ESP32 Link (USART1)
  if (ESP32_Init() != ESP32_OK) {
    UART2_OutString("[ERROR] ESP32 not responding! Check PA9/PA10 wiring.\r\n");
    SSD1306_SetCursor(0, 4);
    SSD1306_OutString("ESP32: NOT FOUND!");
    SSD1306_SetCursor(0, 6);
    SSD1306_OutString("Check PA9/PA10 wires");
    SSD1306_DisplayBuffer();
    while (1) {
      LED_Toggle();
      DelayMs(500);
    }
  }

  UART2_OutString("[INIT] ESP32 ping OK!\r\n");
  SSD1306_SetCursor(0, 3);
  SSD1306_OutString("ESP32: Connected");
  SSD1306_SetCursor(0, 5);
  SSD1306_OutString("Connecting Wi-Fi...");
  SSD1306_DisplayBuffer();

  // 4. Connect to Wi-Fi
  UART2_OutString("[WIFI] Connecting to SSID: ");
  UART2_OutString(WIFI_SSID);
  UART2_OutString(" ...\r\n");

  if (ESP32_ConnectWiFi(WIFI_SSID, WIFI_PASS, ipAddr, sizeof(ipAddr), 20000) !=
      ESP32_OK) {
    UART2_OutString("[ERROR] Wi-Fi connection failed! Update SSID/PASS.\r\n");
    SSD1306_Clear();
    SSD1306_SetCursor(0, 1);
    SSD1306_OutString("Wi-Fi Connection");
    SSD1306_SetCursor(0, 3);
    SSD1306_OutString("FAILED!");
    SSD1306_SetCursor(0, 5);
    SSD1306_OutString("Check SSID/PASS");
    SSD1306_DisplayBuffer();
    while (1) {
      LED_Toggle();
      DelayMs(250);
    }
  }

  UART2_OutString("[WIFI] Connected successfully! IP: ");
  UART2_OutString(ipAddr);
  UART2_OutString("\r\n\r\n");

  SSD1306_Clear();
  SSD1306_SetCursor(0, 0);
  SSD1306_OutString("Wi-Fi Connected!");
  SSD1306_SetCursor(0, 2);
  SSD1306_OutString("IP: ");
  SSD1306_OutString(ipAddr);
  SSD1306_SetCursor(0, 5);
  SSD1306_OutString("Fetching weather...");
  SSD1306_DisplayBuffer();

  DelayMs(1500);

  // 5. Main Weather Query Loop
  while (1) {
    UART2_OutString("------------------------------------------\r\n");
    UART2_OutString("[QUERY] Requesting current weather from ESP32...\r\n");

    SSD1306_Clear();
    SSD1306_SetCursor(0, 0);
    SSD1306_OutString("=== IOT WEATHER ===");
    SSD1306_SetCursor(0, 3);
    SSD1306_OutString("Fetching data...");
    SSD1306_DisplayBuffer();

    LED_On();
    int res = ESP32_GetWeather(NULL, city, temp, desc, 12000);
    LED_Off();

    if (res == ESP32_OK) {
      // Log to PC Console
      UART2_OutString("[SUCCESS] Weather report received:\r\n");
      UART2_OutString("  Location:    ");
      UART2_OutString(city);
      UART2_OutString("\r\n");
      UART2_OutString("  Temperature: ");
      UART2_OutString(temp);
      UART2_OutString(" C\r\n");
      UART2_OutString("  Condition:   ");
      UART2_OutString(desc);
      UART2_OutString("\r\n");

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
      UART2_OutString("[ERROR] Weather query failed or timed out!\r\n");
      SSD1306_Clear();
      SSD1306_SetCursor(0, 1);
      SSD1306_OutString("Weather Query Error");
      SSD1306_SetCursor(0, 3);
      SSD1306_OutString("Check ESP32 / WiFi");
      SSD1306_SetCursor(0, 5);
      SSD1306_OutString("Press B1 to retry");
      SSD1306_DisplayBuffer();
    }

    // Wait 60 seconds before next auto-query, or refresh immediately on button
    // press
    UART2_OutString(
        "[INFO] Press PC13 blue button to refresh immediately.\r\n");
    for (int sec = 0; sec < 600; sec++) { // 600 * 100ms = 60s
      if (Button_IsPressed()) {
        DelayMs(50); // Debounce
        if (Button_IsPressed()) {
          while (Button_IsPressed()) {
            DelayMs(10);
          }
          UART2_OutString(
              "[USER] Button pressed! Refreshing weather data...\r\n");
          break;
        }
      }
      DelayMs(100);
    }
  }
}
