#ifndef ESP32_AT_H
#define ESP32_AT_H

#include <stdint.h>

void ESP32_Init(void);

int ESP32_ConnectWiFi(const char* ssid, const char* pw);

int ESP32_TCPOpen(const char* host, uint16_t port);

int ESP32_TCPSend(const char* data, uint32_t len);

int ESP32_TCPRecv(char* buf, uint32_t maxlen);

void ESP32_TCPClose(void);

#endif /* ESP32_AT_H */