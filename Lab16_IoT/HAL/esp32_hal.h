// esp32_hal.h — Lab 16 IoT Custom Communication Protocol Driver (HAL Version)
//
// Driver between STM32F446RE (Host) and NodeMCU ESP32 (Wi-Fi Coprocessor) using STM32 HAL.

#ifndef ESP32_HAL_H
#define ESP32_HAL_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define ESP32_OK         0
#define ESP32_TIMEOUT   -1
#define ESP32_ERROR     -2

// Register UART handles (huart_esp = USART1 for ESP32, huart_pc = USART2 for PC Terminal)
void ESP32_HAL_Init(UART_HandleTypeDef *huart_esp, UART_HandleTypeDef *huart_pc);

// Test communication liveness (sends "PING\n", expects "PONG\n")
int ESP32_HAL_Ping(uint32_t timeoutMs);

// Connect ESP32 to Wi-Fi Access Point
int ESP32_HAL_ConnectWiFi(const char *ssid, const char *pass, char *ipOut, uint16_t ipMaxLen, uint32_t timeoutMs);

// Query connection status from ESP32
int ESP32_HAL_GetStatus(char *statusOut, uint16_t maxLen, uint32_t timeoutMs);

// Fetch current weather (city can be NULL to query the default configured location)
// Fills cityOut, tempOut ("21.5"), descOut ("Clear sky")
int ESP32_HAL_GetWeather(const char *city, char *cityOut, char *tempOut, char *descOut, uint32_t timeoutMs);

// Helpers for printing to PC Terminal via huart_pc
void UART_PC_Print(const char *str);
void UART_PC_Println(const char *str);

#endif // ESP32_HAL_H