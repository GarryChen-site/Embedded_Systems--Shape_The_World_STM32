// esp32.h — Lab 16 IoT Custom Communication Protocol Driver
//
// Protocol Interface between STM32F446RE (Host) and NodeMCU ESP32 (Wi-Fi Coprocessor)
//
// Command set:
//   - "PING\n"                   -> "PONG"
//   - "CONNECT:<SSID>,<PASS>\n"  -> "WIFI:CONNECTED,<IP>" or "WIFI:FAIL,<REASON>"
//   - "STATUS\n"                 -> "STATUS:CONNECTED,<IP>" or "STATUS:DISCONNECTED"
//   - "WEATHER:<CITY>\n"         -> "WEATHER:OK,<CITY>,<TEMP_C>,<CONDITION>" or "WEATHER:ERR,<MSG>"
//   - "GET:<URL>\n"              -> "DATA:START" ... "DATA:END"

#ifndef ESP32_H
#define ESP32_H

#include <stdint.h>

#define ESP32_OK         0
#define ESP32_TIMEOUT   -1
#define ESP32_ERROR     -2

// Initialize UART1 link and verify ESP32 coprocessor presence
int ESP32_Init(void);

// Test communication liveness (sends "PING", expects "PONG")
int ESP32_Ping(uint32_t timeoutMs);

// Connect ESP32 to Wi-Fi Access Point
// Returns ESP32_OK on success and copies IP into ipOut if non-NULL
int ESP32_ConnectWiFi(const char *ssid, const char *pass, char *ipOut, uint16_t ipMaxLen, uint32_t timeoutMs);

// Query connection status from ESP32
int ESP32_GetStatus(char *statusOut, uint16_t maxLen, uint32_t timeoutMs);

// Fetch current weather (city can be NULL to query the configured location)
// Fills cityOut, tempOut ("21.5"), descOut ("Clear sky" / "Partly cloudy")
// Returns ESP32_OK on success
int ESP32_GetWeather(const char *city, char *cityOut, char *tempOut, char *descOut, uint32_t timeoutMs);

#endif // ESP32_H
