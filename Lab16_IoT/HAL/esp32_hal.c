

#include "esp32_hal.h"
#include <string.h>

static UART_HandleTypeDef *p_huart_esp = NULL;
static UART_HandleTypeDef *p_huart_pc = NULL;

static void UART_ESP_Flush(void) {
  if (!p_huart_esp) {
    return;
  }
  uint8_t dummy;
  while (HAL_UART_Receive(p_huart_esp, &dummy, 1, 5) == HAL_OK) {
    // Drain pending characters
  }
}

static int UART_ESP_ReadLine(char *buf, uint16_t maxLen, uint32_t timeoutMs) {
  if (!p_huart_esp || !buf || maxLen < 2) {
    return 0;
  }

  uint16_t idx = 0;
  uint32_t start = HAL_GetTick();

  while ((HAL_GetTick() - start) < timeoutMs) {
    uint8_t c = 0;
    if (HAL_UART_Receive(p_huart_esp, &c, 1, 10) == HAL_OK) {
      if (c == '\r') {
        continue; // Ignore carriage return
      }
      if (c == '\n') {
        if (idx > 0) {
          buf[idx] = '\0';
          return 1; // Complete line received
        }
        continue; // Skip leading blank lines
      }
      if (idx < maxLen - 1) {
        buf[idx++] = (char)c;
      }
    }
  }

  if (idx > 0) {
    buf[idx] = '\0';
    return 1;
  }
  buf[0] = '\0';
  return 0; // Timeout
}

void ESP32_HAL_Init(UART_HandleTypeDef *huart_esp,
                    UART_HandleTypeDef *huart_pc) {
  p_huart_esp = huart_esp;
  p_huart_pc = huart_pc;
  UART_ESP_Flush();
}

int ESP32_HAL_Ping(uint32_t timeoutMs) {
  if (!p_huart_esp) {
    return ESP32_ERROR;
  }

  char rxLine[64];
  UART_ESP_Flush();
  const char *cmd = "PING\n";
  HAL_UART_Transmit(p_huart_esp, (const uint8_t *)cmd, strlen(cmd), 100);

  if (UART_ESP_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
    if (strcmp(rxLine, "PONG") == 0) {
      return ESP32_OK;
    }
  }
  return ESP32_TIMEOUT;
}

int ESP32_HAL_ConnectWiFi(const char *ssid, const char *pass, char *ipOut,
                          uint16_t ipMaxLen, uint32_t timeoutMs) {
  if (!p_huart_esp || !ssid || !pass) {
    return ESP32_ERROR;
  }

  char rxLine[64];
  UART_ESP_Flush();
  char cmdBuf[128];
  snprintf(cmdBuf, sizeof(cmdBuf), "CONNECT:%s,%s\n", ssid, pass);
  HAL_UART_Transmit(p_huart_esp, (const uint8_t *)cmdBuf, strlen(cmdBuf), 500);

  if (UART_ESP_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
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

int ESP32_HAL_GetStatus(char *statusOut, uint16_t maxLen, uint32_t timeoutMs) {
  if (!p_huart_esp) {
    return ESP32_ERROR;
  }

  char rxLine[64];
  UART_ESP_Flush();
  const char *cmd = "STATUS\n";
  HAL_UART_Transmit(p_huart_esp, (const uint8_t *)cmd, strlen(cmd), 100);

  if (UART_ESP_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
    if (statusOut && maxLen > 0) {
      strncpy(statusOut, rxLine, maxLen - 1);
      statusOut[maxLen - 1] = '\0';
    }
    return ESP32_OK;
  }
  return ESP32_TIMEOUT;
}

int ESP32_HAL_GetWeather(const char *city, char *cityOut, char *tempOut,
                         char *descOut, uint32_t timeoutMs) {
  if (!p_huart_esp) {
    return ESP32_ERROR;
  }

  UART_ESP_Flush();
  char cmdBuf[64];
  if (city && city[0] != '\0') {
    snprintf(cmdBuf, sizeof(cmdBuf), "WEATHER:%s\n", city);
  } else {
    snprintf(cmdBuf, sizeof(cmdBuf), "WEATHER\n");
  }
  HAL_UART_Transmit(p_huart_esp, (const uint8_t *)cmdBuf, strlen(cmdBuf), 200);

  if (UART_ESP_ReadLine(rxLine, sizeof(rxLine), timeoutMs)) {
    // Expected format: "WEATHER:OK,<city>,<temp>,<condition>"
    if (strncmp(rxLine, "WEATHER:OK,", 11) == 0) {
      char *ptr = rxLine + 11;

      // 1. City
      char *nextComma = strchr(ptr, ',');
      if (!nextComma) {
        return ESP32_ERROR;
      }
      *nextComma = '\0';
      if (cityOut) {
        strcpy(cityOut, ptr);
      }

      // 2. Temp
      ptr = nextComma + 1;
      nextComma = strchr(ptr, ',');
      if (!nextComma) {
        return ESP32_ERROR;
      }
      *nextComma = '\0';
      if (tempOut) {
        strcpy(tempOut, ptr);
      }

      // 3. Condition
      ptr = nextComma + 1;
      if (descOut) {
        strcpy(descOut, ptr);
      }

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

void UART_PC_Print(const char *str) {
  if (!p_huart_pc || !str) {
    return;
  }
  HAL_UART_Transmit(p_huart_pc, (const uint8_t *)str, strlen(str), 1000);
}

void UART_PC_Println(const char *str) {
  if (!p_huart_pc) {
    return;
  }
  if (str) {
    HAL_UART_Transmit(p_huart_pc, (const uint8_t *)str, strlen(str), 1000);
  }
  HAL_UART_Transmit(p_huart_pc, (const uint8_t *)"\r\n", 2, 100);
}