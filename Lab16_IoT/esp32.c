// esp32.c — Lab 16 IoT Custom Communication Protocol Driver
//
// Implements commands and responses over UART1 between STM32 and NodeMCU ESP32

#include "esp32.h"
#include "uart.h"
#include <string.h>

#define RX_LINE_BUF_SIZE 256
static char rxLine[RX_LINE_BUF_SIZE];

int ESP32_Init(void) {
  UART1_Init();
  UART1_FlushRx();
  DelayMs(200);

  // Try pinging ESP32
  for (int retry = 0; retry < 5; retry++) {
    if (ESP32_Ping(1000) == ESP32_OK) {
      return ESP32_OK;
    }
    DelayMs(200);
  }
  return ESP32_TIMEOUT;
}

int ESP32_Ping(uint32_t timeoutMs) {
  UART1_FlushRx();
  UART1_OutString("PING\n");

  if (UART1_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
    if (strcmp(rxLine, "PONG") == 0) {
      return ESP32_OK;
    }
  }
  return ESP32_TIMEOUT;
}

int ESP32_ConnectWiFi(const char *ssid, const char *pass, char *ipOut,
                      uint16_t ipMaxLen, uint32_t timeoutMs) {
  if (!ssid || !pass)
    return ESP32_ERROR;

  UART1_FlushRx();
  UART1_OutString("CONNECT:");
  UART1_OutString(ssid);
  UART1_OutChar(',');
  UART1_OutString(pass);
  UART1_OutChar('\n');

  if (UART1_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
    // Expected response: "WIFI:CONNECTED,<ip>" or "WIFI:FAIL,<reason>"
    if (strncmp(rxLine, "WIFI:CONNECTED", 14) == 0) {
      if (ipOut && ipMaxLen > 0) {
        const char *comma = strchr(rxLine, ',');
        if (comma) {
          strncpy(ipOut, comma + 1, ipMaxLen - 1);
          ipOut[ipMaxLen - 1] = '\0';
        } else {
          strncpy(ipOut, "Connected", ipMaxLen - 1);
          ipOut[ipMaxLen - 1] = '\0';
        }
      }
      return ESP32_OK;
    }
  }
  return ESP32_TIMEOUT;
}

int ESP32_GetStatus(char *statusOut, uint16_t maxLen, uint32_t timeoutMs) {
  UART1_FlushRx();
  UART1_OutString("STATUS\n");

  if (UART1_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
    if (statusOut && maxLen > 0) {
      strncpy(statusOut, rxLine, maxLen - 1);
      statusOut[maxLen - 1] = '\0';
    }
    return ESP32_OK;
  }
  return ESP32_TIMEOUT;
}

int ESP32_GetWeather(const char *city, char *cityOut, char *tempOut,
                     char *descOut, uint32_t timeoutMs) {
  if (!city)
    return ESP32_ERROR;

  UART1_FlushRx();
  UART1_OutString("WEATHER:");
  UART1_OutString(city);
  UART1_OutChar('\n');

  // ESP32 may take several seconds to make the HTTP request and parse JSON
  if (UART1_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
    // Expected format: "WEATHER:OK,<city>,<temp>,<condition>"
    if (strncmp(rxLine, "WEATHER:OK,", 11) == 0) {
      char *ptr = rxLine + 11;

      // 1. City
      char *nextComma = strchr(ptr, ',');
      if (!nextComma)
        return ESP32_ERROR;
      *nextComma = '\0';
      if (cityOut)
        strcpy(cityOut, ptr);

      // 2. Temp
      ptr = nextComma + 1;
      nextComma = strchr(ptr, ',');
      if (!nextComma)
        return ESP32_ERROR;
      *nextComma = '\0';
      if (tempOut)
        strcpy(tempOut, ptr);

      // 3. Condition
      ptr = nextComma + 1;
      if (descOut)
        strcpy(descOut, ptr);

      return ESP32_OK;
    } else if (strncmp(rxLine, "WEATHER:ERR,", 12) == 0) {
      if (descOut) {
        strcpy(descOut, rxLine + 12);
      }
      return ESP32_ERROR;
    }
  }
  return ESP32_TIMEOUT;
}
