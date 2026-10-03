/*
 * esp32_iot_bridge.ino — NodeMCU ESP32 Wi-Fi Coprocessor Firmware
 * Lab 16 IoT: Shape The World STM32 Port
 *
 * Provides a lightweight, robust bridge between STM32F446RE (Host)
 * and OpenWeatherMap One Call API 4.0 over UART2 (GPIO 16/17).
 *
 * Hardware:
 *   - NodeMCU ESP32 / ESP32 DevKit v1
 *   - UART2:
 *       GPIO 16 (RX2) -> Connect to STM32 PA9  (USART1_TX)
 *       GPIO 17 (TX2) -> Connect to STM32 PA10 (USART1_RX)
 *       GND           -> Connect to STM32 GND  (MANDATORY common ground!)
 *   - USB Port: Connected to PC for Arduino IDE serial monitor @ 115200 baud
 *   - Onboard LED: GPIO 2
 *
 * Weather API:
 *   OpenWeatherMap One Call API 4.0 (Current Weather Data)
 *   Endpoint: https://api.openweathermap.org/data/4.0/onecall/current?lat=***&lon=***&units=metric&lang=en&appid=<API_KEY>
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// =========================================================================
// Configuration
// =========================================================================

// Enter your OpenWeatherMap API Key here:
const char* OWM_API_KEY = "YOUR_API_KEY_HERE";

// Fixed endpoint parameters:
// Latitude:  35.7217 (Tokyo)
// Longitude: 139.8269
// Units:     metric
// Lang:      en
const char* OWM_ENDPOINT = "https://api.openweathermap.org/data/4.0/onecall/current?lat=***&lon=***&units=metric&lang=en&appid=";

// UART pins for communication with STM32
#define STM32_RX_PIN  16  // ESP32 RX2 <- STM32 PA9 (TX)
#define STM32_TX_PIN  17  // ESP32 TX2 -> STM32 PA10 (RX)
#define LED_PIN        2  // Onboard blue LED

// =========================================================================
// Prototypes
// =========================================================================
void processCommand(String cmd);
bool connectWiFi(String ssid, String pass);
bool fetchWeather(String requestedCity, String &outCity, String &outTemp, String &outDesc, String &outErr);

// =========================================================================
// Setup
// =========================================================================
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // USB Serial for PC Debug Console
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n==========================================");
  Serial.println("  NodeMCU ESP32 IoT Bridge for Lab 16");
  Serial.println("  OpenWeatherMap One Call API 4.0");
  Serial.println("==========================================");

  // Serial2 for STM32 Link (8N1 @ 115200 baud)
  Serial2.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  Serial.println("[ESP32] Serial2 initialized on RX=16, TX=17.");
  Serial.println("[ESP32] Awaiting commands from STM32...");
}

// =========================================================================
// Main Loop
// =========================================================================
void loop() {
  // Check for commands from STM32 (Serial2)
  if (Serial2.available()) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      Serial.print("[STM32 -> ESP32] ");
      Serial.println(cmd);
      processCommand(cmd);
    }
  }

  // Also allow commands from USB Serial for manual testing
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      Serial.print("[USB -> ESP32] ");
      Serial.println(cmd);
      processCommand(cmd);
    }
  }
}

// =========================================================================
// Command Processor
// =========================================================================
void processCommand(String cmd) {
  if (cmd.equalsIgnoreCase("PING")) {
    Serial2.println("PONG");
    Serial.println("[ESP32 -> STM32] PONG");
  }
  else if (cmd.startsWith("CONNECT:")) {
    // Format: "CONNECT:<SSID>,<PASS>"
    String payload = cmd.substring(8);
    int commaIdx = payload.indexOf(',');
    if (commaIdx != -1) {
      String ssid = payload.substring(0, commaIdx);
      String pass = payload.substring(commaIdx + 1);
      ssid.trim();
      pass.trim();

      Serial.print("[ESP32] Connecting to Wi-Fi SSID: '");
      Serial.print(ssid);
      Serial.println("'...");

      if (connectWiFi(ssid, pass)) {
        String resp = "WIFI:CONNECTED," + WiFi.localIP().toString();
        Serial2.println(resp);
        Serial.println("[ESP32 -> STM32] " + resp);
      } else {
        Serial2.println("WIFI:FAIL,TIMEOUT");
        Serial.println("[ESP32 -> STM32] WIFI:FAIL,TIMEOUT");
      }
    } else {
      Serial2.println("WIFI:FAIL,INVALID_SYNTAX");
    }
  }
  else if (cmd.equalsIgnoreCase("STATUS")) {
    if (WiFi.status() == WL_CONNECTED) {
      String resp = "STATUS:CONNECTED," + WiFi.localIP().toString();
      Serial2.println(resp);
      Serial.println("[ESP32 -> STM32] " + resp);
    } else {
      Serial2.println("STATUS:DISCONNECTED");
      Serial.println("[ESP32 -> STM32] STATUS:DISCONNECTED");
    }
  }
  else if (cmd.startsWith("WEATHER:") || cmd.equalsIgnoreCase("WEATHER")) {
    String cityParam = "";
    if (cmd.startsWith("WEATHER:")) {
      cityParam = cmd.substring(8);
      cityParam.trim();
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial2.println("WEATHER:ERR,WIFI_NOT_CONNECTED");
      Serial.println("[ESP32 -> STM32] WEATHER:ERR,WIFI_NOT_CONNECTED");
      return;
    }

    if (strcmp(OWM_API_KEY, "YOUR_API_KEY_HERE") == 0 || strlen(OWM_API_KEY) == 0) {
      Serial.println("[ESP32] Error: OpenWeatherMap API key not set in OWM_API_KEY!");
      Serial2.println("WEATHER:ERR,API_KEY_NOT_SET");
      return;
    }

    String outCity, outTemp, outDesc, outErr;
    bool ok = fetchWeather(cityParam, outCity, outTemp, outDesc, outErr);

    if (ok) {
      String resp = "WEATHER:OK," + outCity + "," + outTemp + "," + outDesc;
      Serial2.println(resp);
      Serial.println("[ESP32 -> STM32] " + resp);
    } else {
      String resp = "WEATHER:ERR," + (outErr.length() > 0 ? outErr : "REQUEST_FAILED");
      Serial2.println(resp);
      Serial.println("[ESP32 -> STM32] " + resp);
    }
  }
  else {
    Serial.println("[ESP32] Unknown command: " + cmd);
    Serial2.println("ERR:UNKNOWN_COMMAND");
  }
}

// =========================================================================
// Wi-Fi Connection
// =========================================================================
bool connectWiFi(String ssid, String pass) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.begin(ssid.c_str(), pass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Blink LED while connecting
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH); // Solid ON when connected
    Serial.println("\r\n[ESP32] Wi-Fi connected! IP: " + WiFi.localIP().toString());
    return true;
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("\r\n[ESP32] Wi-Fi connection timed out!");
    return false;
  }
}

// =========================================================================
// Fetch Weather via OpenWeatherMap One Call API 4.0
// =========================================================================
bool fetchWeather(String requestedCity, String &outCity, String &outTemp, String &outDesc, String &outErr) {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification for IoT query

  String url = String(OWM_ENDPOINT) + String(OWM_API_KEY);
  Serial.println("[HTTP] Requesting: " + url);

  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTP] Request failed, HTTP code: %d\n", httpCode);
    if (httpCode == 401) {
      outErr = "INVALID_API_KEY";
      Serial.println("[HTTP] 401 Unauthorized: Check your OpenWeatherMap API key!");
    } else {
      outErr = "HTTP_" + String(httpCode);
    }
    http.end();
    client.stop();
    digitalWrite(LED_PIN, (WiFi.status() == WL_CONNECTED) ? HIGH : LOW);
    return false;
  }

  String payload = http.getString();
  http.end();
  client.stop();
  digitalWrite(LED_PIN, HIGH);

  Serial.println("[HTTP] Response received (" + String(payload.length()) + " bytes)");

  // --- 1. Parse Temperature ---
  // Structure: "data":[{"dt":..., "temp":24.52, ...}]
  int tempIdx = payload.indexOf("\"temp\":");
  if (tempIdx != -1) {
    int tempEnd = payload.indexOf(",", tempIdx + 7);
    if (tempEnd == -1) tempEnd = payload.indexOf("}", tempIdx + 7);
    if (tempEnd != -1) {
      String rawTemp = payload.substring(tempIdx + 7, tempEnd);
      rawTemp.trim();
      float t = rawTemp.toFloat();
      outTemp = String(t, 1); // 1 decimal place (e.g. "24.5")
    } else {
      outTemp = "--";
    }
  } else {
    outTemp = "--";
  }

  // --- 2. Parse Weather Description ---
  // Structure: "weather":[{"id":800,"main":"Clear","description":"sky is clear",...}]
  int descIdx = payload.indexOf("\"description\":\"");
  if (descIdx != -1) {
    int descEnd = payload.indexOf("\"", descIdx + 15);
    if (descEnd != -1) {
      outDesc = payload.substring(descIdx + 15, descEnd);
    } else {
      outDesc = "Clear";
    }
  } else {
    int mainIdx = payload.indexOf("\"main\":\"");
    if (mainIdx != -1) {
      int mainEnd = payload.indexOf("\"", mainIdx + 8);
      outDesc = payload.substring(mainIdx + 8, mainEnd);
    } else {
      outDesc = "Clear";
    }
  }

  // --- 3. Determine Location / City Name ---
  // If timezone exists (e.g. "Asia/Tokyo"), extract city name "Tokyo"
  int tzIdx = payload.indexOf("\"timezone\":\"");
  if (tzIdx != -1) {
    int tzEnd = payload.indexOf("\"", tzIdx + 12);
    if (tzEnd != -1) {
      String tz = payload.substring(tzIdx + 12, tzEnd);
      int slashIdx = tz.indexOf('/');
      if (slashIdx != -1) {
        outCity = tz.substring(slashIdx + 1);
      } else {
        outCity = tz;
      }
    } else {
      outCity = "Tokyo";
    }
  } else if (requestedCity.length() > 0) {
    outCity = requestedCity;
  } else {
    outCity = "Tokyo";
  }

  Serial.printf("[WEATHER] Parsed -> City: %s, Temp: %s C, Condition: %s\n",
                outCity.c_str(), outTemp.c_str(), outDesc.c_str());
  return true;
}
