# Lab 16 — IoT Weather Station with ESP32 & SSD1306 OLED (HAL Version)

Connects the **STM32F446RE Nucleo** to a **NodeMCU ESP32** acting as a Wi-Fi / HTTPS coprocessor over UART, queries live current weather from the **OpenWeatherMap One Call 4.0 API**, and renders the temperature, location, and weather condition on a 0.96" SSD1306 OLED and PC Serial Console using the **STM32 HAL library (STM32CubeIDE)**.

---

## System Architecture

```
+-------------------+                    +--------------------+
|                   |   USART2 (115200)  |  PC Serial Monitor |
|                   |<==================>| (115200 8N1 / Putty|
|                   |   (ST-Link USB)    +--------------------+
|                   |
|   STM32F446RE     |   I2C1 (400 kHz)   +--------------------+
|   Nucleo-64       |------------------->| SSD1306 128x64     |
|   (Master MCU)    |   PB6=SCL, PB7=SDA | OLED Display       |
|                   |                    +--------------------+
|                   |   USART1 (115200)  +--------------------+                    +---------------------+
|                   |<==================>|   NodeMCU ESP32    |<--- Wi-Fi / HTTPS | OpenWeatherMap      |
|                   |   PA9(TX) -> RX2   | (Network Coproc.)  |                    | One Call 4.0 API    |
+-------------------+   PA10(RX)<- TX2   +--------------------+                    +---------------------+
```

---

## Hardware Wiring

### 1. STM32 Nucleo $\leftrightarrow$ NodeMCU ESP32 (UART Link)

| STM32 Pin | Nucleo Header Location | ESP32 Pin | ESP32 Function | Notes |
|:---|:---|:---|:---|:---|
| **PA9** (TX1) | CN10 pin 21 (D8) | **GPIO 16** | RX2 (ESP32 Receive) | Cross TX $\rightarrow$ RX |
| **PA10** (RX1) | CN10 pin 33 (D2) | **GPIO 17** | TX2 (ESP32 Transmit) | Cross RX $\leftarrow$ TX |
| **GND** | Any GND (e.g. CN10 pin 20) | **GND** | Ground | **Mandatory common ground** |

> **Powering the ESP32:**  
> Power the NodeMCU ESP32 using its own micro-USB cable connected to your PC or a 5V USB charger.  
> Connecting common GND between the Nucleo and ESP32 is required for reliable UART signal levels.

---

### 2. STM32 Nucleo $\leftrightarrow$ SSD1306 0.96" OLED (I2C1)

| OLED Pin | STM32 Pin | Nucleo Header Location | Notes |
|:---|:---|:---|:---|
| **GND** | GND | Any GND (e.g. CN6 pin 6 / CN10 pin 9) | Power ground |
| **VCC** | 3.3V | CN6 pin 4 (3V3) | 3.3V power |
| **SCL** | **PB6** (default) or PB8 | CN10 pin 17 (D11) or CN10 pin 3 (D15) | I2C1 Clock (Fast Mode 400 kHz) |
| **SDA** | **PB7** (default) or PB9 | CN10 pin 19 (D4) or CN10 pin 5 (D14) | I2C1 Data |

> **No external pull-up resistors required:** The SSD1306 breakout board includes on-board $4.7\,\text{k}\Omega$ or $10\,\text{k}\Omega$ pull-up resistors to 3.3V.

---

### 3. Nucleo On-Board Controls

| Function | Nucleo Pin | Description |
|:---|:---|:---|
| **User LED (LD2)** | **PA5** | Green LED. Turned ON during network queries; blinks on error. |
| **User Button (B1)** | **PC13** | Blue push button (active LOW). Press to trigger an immediate weather query. |
| **Virtual COM Port** | **PA2 (TX), PA3 (RX)** | Routed directly through ST-LINK USB to PC at 115200 baud. |

---

## STM32CubeIDE IOC Configuration

### Step 1 — Create a New STM32 Project

1. Open **STM32CubeIDE** $\rightarrow$ **File $\rightarrow$ New $\rightarrow$ STM32 Project**.
2. In the Board Selector tab, search for `NUCLEO-F446RE` $\rightarrow$ Select it $\rightarrow$ Click **Next**.
3. Set **Project Name**: `Lab16_IoT_HAL`
4. Targeted Language: **C**
5. Click **Finish**. When prompted *"Initialize all peripherals with their default mode?"*, click **No** (start clean).

---

### Step 2 — System Core $\rightarrow$ SYS (Debug)

1. In the `.ioc` Pinout & Configuration view:
2. Navigate to **System Core $\rightarrow$ SYS**.
3. Set **Debug** = `Serial Wire`.  
   *(Ensures PA13/PA14 are kept for the ST-Link debugger).*

---

### Step 3 — Clock Configuration

Use **HSI 16 MHz**, no PLL (identical to Lab 14 and Lab 15):

| Field | Value |
|:---|:---|
| **Input frequency** | HSI 16 MHz |
| **PLL** | OFF |
| **System Clock Mux (SYSCLK)** | HSI (16 MHz) |
| **AHB Prescaler (HCLK)** | /1 (16 MHz) |
| **APB1 Prescaler (PCLK1)** | /1 (16 MHz) |
| **APB2 Prescaler (PCLK2)** | /1 (16 MHz) |

> **Note:** The HAL I2C and UART initialization functions automatically compute their internal baud rate / timing registers based on the bus clock (PCLK1 / PCLK2). Operating at 16 MHz keeps things simple and matches the generated `SystemClock_Config()` in `main.c`.

---

### Step 4 — Connectivity $\rightarrow$ USART2 (PC Terminal / ST-Link VCP)

1. Navigate to **Connectivity $\rightarrow$ USART2**.
2. Set **Mode** = `Asynchronous`.
3. In **Configuration $\rightarrow$ Parameter Settings**:
   - **Baud Rate**: `115200`
   - **Word Length**: `8 Bits`
   - **Parity**: `None`
   - **Stop Bits**: `1`
   - **Data Direction**: `Receive and Transmit`
4. Confirm pins in Pinout view: `PA2 = USART2_TX`, `PA3 = USART2_RX`.

---

### Step 5 — Connectivity $\rightarrow$ USART1 (ESP32 Coprocessor Link)

1. Navigate to **Connectivity $\rightarrow$ USART1**.
2. Set **Mode** = `Asynchronous`.
3. In **Configuration $\rightarrow$ Parameter Settings**:
   - **Baud Rate**: `115200`
   - **Word Length**: `8 Bits`
   - **Parity**: `None`
   - **Stop Bits**: `1`
   - **Data Direction**: `Receive and Transmit`
4. In the Pinout view, verify:
   - `PA9` = `USART1_TX`
   - `PA10` = `USART1_RX`

---

### Step 6 — Connectivity $\rightarrow$ I2C1 (SSD1306 OLED)

1. Navigate to **Connectivity $\rightarrow$ I2C1**.
2. Set **Mode** = `I2C`.
3. In **Configuration $\rightarrow$ Parameter Settings**:
   - **I2C Speed Mode**: `Fast Mode`
   - **I2C Clock Speed (Hz)**: `400000`
   - **I2C Duty Cycle in Fast Mode**: `Duty cycle Tlow/Thigh = 2`
   - All other fields: leave as default (7-bit, no dual address).
4. Verify pins in Pinout view:
   - `PB6` = `I2C1_SCL`
   - `PB7` = `I2C1_SDA`  
   *(If you prefer Arduino headers D15/D14, remap PB6/PB7 to PB8/PB9)*.

---

### Step 7 — System Core $\rightarrow$ GPIO (LED & User Button)

1. Click pin **PA5** in the chip view $\rightarrow$ Set to `GPIO_Output`.
   - In **System Core $\rightarrow$ GPIO $\rightarrow$ PA5**:
     - GPIO Output Level: `Low`
     - GPIO mode: `Output Push Pull`
     - Pull-up/Pull-down: `No pull-up and no pull-down`
     - Maximum output speed: `Low`
     - User Label: `LD2`
2. Click pin **PC13** in the chip view $\rightarrow$ Set to `GPIO_Input`.
   - In **System Core $\rightarrow$ GPIO $\rightarrow$ PC13**:
     - GPIO mode: `Input mode`
     - Pull-up/Pull-down: `Pull-up`
     - User Label: `B1_USER`

---

### Step 8 — Project Manager Settings & Code Generation

1. Click the **Project Manager** tab.
2. Under **Project**:
   - Toolchain / IDE: `STM32CubeIDE`
3. Under **Code Generator**:
   - Check **Generate peripheral initialization as a pair of '.c/.h' files per peripheral**.
   - Check **Keep user code when re-generating**.
4. Press **Ctrl+S** (or click the Save icon).
5. When prompted *"Do you want to generate Code?"*, click **Yes**.

---

## File Structure & Integration

Copy the driver files from `Lab16_IoT/HAL/` into your CubeIDE project directory:

```
Lab16_IoT_HAL/
├── Core/
│   ├── Inc/
│   │   ├── esp32_hal.h       <-- [COPY HERE]
│   │   ├── ssd1306_hal.h     <-- [COPY HERE]
│   │   └── main.h
│   └── Src/
│       ├── esp32_hal.c       <-- [COPY HERE]
│       ├── ssd1306_hal.c     <-- [COPY HERE]
│       └── main.c            <-- [MERGE or REPLACE with HAL/main.c]
└── Drivers/
    └── STM32F4xx_HAL_Driver/
```

> **Tip:** You can either copy `Lab16_IoT/HAL/main.c` directly into `Core/Src/main.c`.



---

## ESP32 Coprocessor Firmware Setup

1. Open `Lab16_IoT/esp32_bridge/esp32_iot_bridge.ino` in the **Arduino IDE**.
2. Verify that your OpenWeatherMap API key is set:
   ```cpp
   const char* OPENWEATHER_API_KEY = "YOUR_API_KEY";
   ```
3. Connect the NodeMCU ESP32 via USB and click **Upload**.
4. Once flashed, connect the NodeMCU ESP32 to the STM32 Nucleo headers using the UART wiring table above.

---

## Build and Run

1. In STM32CubeIDE, click **Project $\rightarrow$ Build All** (or `Ctrl+B`).
2. Verify that the build succeeds with **0 errors**.
3. Connect the STM32F446RE Nucleo board to your PC via ST-LINK USB.
4. Click **Run $\rightarrow$ Debug** (`F11`) $\rightarrow$ Click **Resume** (`F8`) to begin execution.
5. Open your preferred serial terminal (PuTTY, Tera Term, or Arduino Serial Monitor) at:
   - **Port**: STMicroelectronics STLink Virtual COM Port
   - **Baud rate**: `115200`
   - **Data**: 8 bits, No parity, 1 stop bit

---

## OLED & Terminal Output

### OLED Display Layout:
```
+------------------------+
| === IOT WEATHER ===    |  row 0 (Title banner)
|                        |  row 1
| City: Tokyo            |  row 2 (Location)
|                        |  row 3
| Temp: 22.4 C           |  row 4 (Temperature)
|                        |  row 5
| clear sky              |  row 6 (Condition summary)
|                        |  row 7
+------------------------+
```

### Serial Monitor Output:
```text
==========================================
  Lab 16: IoT Weather Station (STM32 HAL) 
==========================================
[INIT] Connecting to ESP32 coprocessor...
[INIT] ESP32 ping OK!
[WIFI] Connecting to SSID: MyHomeWiFi ...
[WIFI] Connected successfully! IP: 192.168.1.105

------------------------------------------
[QUERY] Requesting current weather from ESP32...
[SUCCESS] Weather report received:
  Location:    Tokyo
  Temperature: 22.4 C
  Condition:   clear sky
[INFO] Press PC13 blue button to refresh immediately.
```

---

## Troubleshooting

| Symptom | Probable Cause | Solution |
|:---|:---|:---|
| **OLED stays black / blank** | Incorrect I2C address or wiring | Ensure GND and 3.3V are securely connected. Check whether SA0 on your OLED is pulled to VCC (in that case, change `SSD1306_I2C_ADDR_7` in `ssd1306_hal.h` to `0x3D`). |
| **"ESP32: NOT FOUND!" on OLED** | Crossed or loose UART wires | Verify PA9 (TX1) connects to ESP32 RX2 (GPIO 16) and PA10 (RX1) connects to ESP32 TX2 (GPIO 17). Ensure GND is shared between both boards. |
| **"Wi-Fi Connection FAILED!"** | Incorrect Wi-Fi SSID / password | Update `WIFI_SSID` and `WIFI_PASS` in `main.c` (or ensure the ESP32 is within Wi-Fi range). Note: ESP32 only supports 2.4 GHz Wi-Fi networks. |
| **"Weather Query Error"** | API Key invalid or network timeout | Verify that your OpenWeatherMap key is activated for One Call 4.0 API. Test standalone with `esp32_weather_test.ino` first. |
| **Code lost after regenerating IOC** | Code was placed outside USER CODE tags | Always insert custom code between `/* USER CODE BEGIN ... */` and `/* USER CODE END ... */`. |
