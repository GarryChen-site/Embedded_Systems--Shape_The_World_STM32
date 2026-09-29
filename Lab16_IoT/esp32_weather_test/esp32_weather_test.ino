/*
 * esp32_weather_test.ino — Standalone Weather Test for NodeMCU ESP32
 *
 * Use this sketch to test OpenWeatherMap One Call API 4.0 directly
 * from the Arduino IDE Serial Monitor WITHOUT needing the STM32 connected.
 *
 * Instructions:
 *   1. Enter your Wi-Fi SSID, Password, and OpenWeatherMap API Key below.
 *   2. Connect NodeMCU ESP32 to PC via USB.
 *   3. Open Arduino IDE, select your ESP32 board and COM port.
 *   4. Click Upload.
 *   5. Open Serial Monitor at 115200 baud (set line ending to "Newline").
 *   6. You can type 'r' and press Enter in the Serial Monitor to re-query at any time.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>

// =========================================================================
// Configuration — Fill in your credentials here
// =========================================================================
const char* WIFI_SSID   = "YOUR_WIFI_SSID";
const char* WIFI_PASS   = "YOUR_WIFI_PASSWORD";
const char* OWM_API_KEY = "YOUR_API_KEY_HERE";

// Fixed endpoint: lat=***, lon=***, metric, lang=en
const char* OWM_ENDPOINT = "https://api.openweathermap.org/data/4.0/onecall/current?lat=***&lon=***&units=metric&lang=en&appid=";

#define LED_PIN 2 // Onboard LED

// =========================================================================
// Function Prototypes
// =========================================================================
void runWeatherTest();
void printBootDiagnostics();

// =========================================================================
// Setup
// =========================================================================
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(1500); // Allow USB Serial Monitor time to connect

  Serial.println("\r\n==========================================");
  Serial.println("  ESP32 Standalone Weather API Test");
  Serial.println("  OpenWeatherMap One Call 4.0");
  Serial.println("==========================================");

  printBootDiagnostics();

  // Check if credentials are still placeholder
  bool configError = false;
  if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0 || strlen(WIFI_SSID) == 0) {
    Serial.println("\r\n[!] CONFIG ERROR: WIFI_SSID is not set!");
    Serial.println("    Please replace 'YOUR_WIFI_SSID' with your actual Wi-Fi name (line 24).");
    configError = true;
  }
  if (strcmp(OWM_API_KEY, "YOUR_API_KEY_HERE") == 0 || strlen(OWM_API_KEY) == 0) {
    Serial.println("\r\n[!] CONFIG ERROR: OWM_API_KEY is not set!");
    Serial.println("    Please replace 'YOUR_API_KEY_HERE' with your actual key (line 26).");
    configError = true;
  }

  if (configError) {
    Serial.println("\r\n[PAUSED] Edit lines 24-26 in the sketch and re-upload.\r\n");
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
  }

  // Connect to Wi-Fi
  Serial.print("\r\n[WIFI] Connecting to '");
  Serial.print(WIFI_SSID);
  Serial.print("'");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println(" CONNECTED!");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WIFI] Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.printf("[SYSTEM] Free Heap: %u bytes\r\n\r\n", ESP.getFreeHeap());

    // Perform initial test query
    runWeatherTest();
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println(" FAILED!");
    Serial.println("[WIFI] Could not connect to Wi-Fi. Check SSID and password.");
  }

  Serial.println("\r\n[INFO] Type 'r' and press Enter in the Serial Monitor to refresh.");
}

// =========================================================================
// Loop
// =========================================================================
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.equalsIgnoreCase("r") || input.equalsIgnoreCase("refresh") || input.length() > 0) {
      Serial.println("\r\n[USER] Refreshing weather data...");
      runWeatherTest();
      Serial.println("\r\n[INFO] Type 'r' and press Enter to refresh again.");
    }
  }
  delay(100); // Prevent watchdog trigger
}

// =========================================================================
// Weather Test Query
// =========================================================================
void runWeatherTest() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] Wi-Fi is disconnected! Cannot perform HTTP request.");
    return;
  }

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation for IoT query

  String fullUrl = String(OWM_ENDPOINT) + String(OWM_API_KEY);
  Serial.println("------------------------------------------");
  Serial.println("[HTTP] Sending GET request to:");
  Serial.println(fullUrl);

  digitalWrite(LED_PIN, LOW);
  unsigned long t0 = millis();
  http.begin(client, fullUrl);
  int httpCode = http.GET();
  unsigned long elapsed = millis() - t0;
  digitalWrite(LED_PIN, HIGH);

  Serial.printf("[HTTP] Completed in %lu ms. HTTP Status Code: %d\n", elapsed, httpCode);

  if (httpCode == HTTP_CODE_OK) { // 200 OK
    String payload = http.getString();
    http.end();
    client.stop();

    Serial.println("\r\n[HTTP] Raw JSON Response:");
    Serial.println("------------------------------------------");
    Serial.println(payload);
    Serial.println("------------------------------------------");

    // --- Parse Fields ---
    // 1. Temperature from "temp":
    String outTemp = "--";
    int tempIdx = payload.indexOf("\"temp\":");
    if (tempIdx != -1) {
      int tempEnd = payload.indexOf(",", tempIdx + 7);
      if (tempEnd == -1) tempEnd = payload.indexOf("}", tempIdx + 7);
      if (tempEnd != -1) {
        String raw = payload.substring(tempIdx + 7, tempEnd);
        raw.trim();
        outTemp = String(raw.toFloat(), 1);
      }
    }

    // 2. Weather Description from "description":"
    String outDesc = "Unknown";
    int descIdx = payload.indexOf("\"description\":\"");
    if (descIdx != -1) {
      int descEnd = payload.indexOf("\"", descIdx + 15);
      if (descEnd != -1) {
        outDesc = payload.substring(descIdx + 15, descEnd);
      }
    } else {
      int mainIdx = payload.indexOf("\"main\":\"");
      if (mainIdx != -1) {
        int mainEnd = payload.indexOf("\"", mainIdx + 8);
        outDesc = payload.substring(mainIdx + 8, mainEnd);
      }
    }

    // 3. Location from "timezone":"
    String outCity = "Tokyo";
    int tzIdx = payload.indexOf("\"timezone\":\"");
    if (tzIdx != -1) {
      int tzEnd = payload.indexOf("\"", tzIdx + 12);
      if (tzEnd != -1) {
        String tz = payload.substring(tzIdx + 12, tzEnd);
        int slash = tz.indexOf('/');
        outCity = (slash != -1) ? tz.substring(slash + 1) : tz;
      }
    }

    // Print Clean Summary
    Serial.println("\r\n==========================================");
    Serial.println("        WEATHER TEST SUCCESSFUL!          ");
    Serial.println("==========================================");
    Serial.printf("  Location:     %s\n", outCity.c_str());
    Serial.printf("  Coordinates:  lat=35.7217, lon=139.8269\n");
    Serial.printf("  Temperature:  %s deg C\n", outTemp.c_str());
    Serial.printf("  Condition:    %s\n", outDesc.c_str());
    Serial.println("==========================================");
    Serial.printf("  Simulated STM32 UART Packet:\n");
    Serial.printf("  -> WEATHER:OK,%s,%s,%s\n", outCity.c_str(), outTemp.c_str(), outDesc.c_str());
    Serial.println("==========================================");

  } else {
    // Error diagnostics
    String errBody = http.getString();
    http.end();
    client.stop();

    Serial.println("\r\n[ERROR] Request failed!");
    if (httpCode == 401) {
      Serial.println("  -> Error 401: Unauthorized!");
      Serial.println("  -> Cause: Your OpenWeatherMap API key is invalid or not yet activated.");
      Serial.println("  -> Note: New OpenWeatherMap keys can take 10-60 minutes to activate after signup.");
    } else if (httpCode == 429) {
      Serial.println("  -> Error 429: Rate limit exceeded.");
    } else if (httpCode < 0) {
      Serial.printf("  -> Connection error: %s\n", http.errorToString(httpCode).c_str());
    } else {
      Serial.printf("  -> Server returned error code: %d\n", httpCode);
    }

    if (errBody.length() > 0) {
      Serial.println("  -> Server response body: " + errBody);
    }
  }
}

// =========================================================================
// Boot Diagnostics
// =========================================================================
void printBootDiagnostics() {
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print("[SYSTEM] Boot Reason: ");
  switch (reason) {
    case ESP_RST_POWERON:   Serial.println("Normal Power-on"); break;
    case ESP_RST_SW:        Serial.println("Software Reset"); break;
    case ESP_RST_PANIC:     Serial.println("CRASH / PANIC (Exception occurred)"); break;
    case ESP_RST_INT_WDT:   Serial.println("Interrupt Watchdog Reset"); break;
    case ESP_RST_TASK_WDT:  Serial.println("Task Watchdog Reset"); break;
    case ESP_RST_BROWNOUT:  Serial.println("BROWNOUT (Power supply dipped below 2.7V!)"); break;
    default:                Serial.printf("Code %d\n", reason); break;
  }
}
