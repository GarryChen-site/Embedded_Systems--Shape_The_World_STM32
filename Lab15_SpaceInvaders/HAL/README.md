# Lab 15 — Space Invaders (HAL Version)

STM32 F446RE port of the original TM4C Space Invaders game.
Display: 0.96" SSD1306 OLED 128×64 (I2C).
Sound: 4-bit R-2R DAC → buzzer (optional).
Control: trimmer pot (player X) + 2 push-buttons (fire).

---

## Hardware Wiring

### Pin Mapping Overview

| Component | Pin | Nucleo connector |
|-----------|-----|-----------------|
| Pot CCW end | GND | Any GND |
| Pot wiper | PA0 | CN8 pin 1 (A0) |
| Pot CW end | 3.3V | CN8 pin 4 |
| OLED GND | GND | Any GND |
| OLED VCC | 3.3V | CN8 pin 4 |
| OLED SCL | **PB6** (default) | CN10 pin 17 (D10) |
| OLED SDA | **PB7** (default) | CN10 pin 19 |
| DAC PB0 (LSB) | PB0 | CN8 pin 5 (A3) |
| DAC PB1 | PB1 | CN10 pin 24 |
| DAC PB2 | PB2 | CN10 pin 22 |
| DAC PB4 (MSB) | PB4 | CN10 pin 27 (D12) |
| Buzzer+ | R-2R output node (A17) | Breadboard A17 |
| Buzzer– | GND | Any GND rail |
| Fire button 1 | PC6 | CN10 pin 13 |
| Fire button 2 | PC8 | CN10 pin 3 |

> **Fire buttons**: normally-open, **active-high**.  
> Wire each button between the pin and **3.3V** with a **10 kΩ pull-down** to GND.  
> OLED module has on-board pull-ups — no external resistors needed on SCL/SDA.

---

### 4-Bit R-2R Resistor Ladder Breadboard Layout

Standard 4-bit R-2R ladder using thirteen (13) **10 kΩ** resistors:
* **$2R = 20\text{ k}\Omega$** implemented by two $10\text{ k}\Omega$ resistors in series.
* **$R = 10\text{ k}\Omega$** implemented by a single $10\text{ k}\Omega$ resistor.

#### Exact Breadboard Resistor Placement (Grid Coordinates)

| Resistor | Type | Coordinates | Purpose |
|:---|:---|:---|:---|
| **R1** | 10 kΩ | `(A1, A2)` | Bit 0 (LSB) input branch — part 1 ($2R$) |
| **R2** | 10 kΩ | `(B2, B3)` | Bit 0 (LSB) input branch — part 2 ($2R$) |
| **R3** | 10 kΩ | `(C3, C4)` | Ladder termination branch — part 1 ($2R$) |
| **R4** | 10 kΩ | `(D4, D5)` | Ladder termination branch — part 2 ($2R$) |
| **R5** | 10 kΩ | `(E3, E9)` | Ladder bridge between Bit 0 and Bit 1 ($R$) |
| **R6** | 10 kΩ | `(A7, A8)` | Bit 1 input branch — part 1 ($2R$) |
| **R7** | 10 kΩ | `(B8, B9)` | Bit 1 input branch — part 2 ($2R$) |
| **R8** | 10 kΩ | `(A11, A12)` | Bit 2 input branch — part 1 ($2R$) |
| **R9** | 10 kΩ | `(B12, B13)` | Bit 2 input branch — part 2 ($2R$) |
| **R10**| 10 kΩ | `(C9, C13)` | Ladder bridge between Bit 1 and Bit 2 ($R$) |
| **R11**| 10 kΩ | `(A15, A16)` | Bit 3 (MSB) input branch — part 1 ($2R$) |
| **R12**| 10 kΩ | `(B16, B17)` | Bit 3 (MSB) input branch — part 2 ($2R$) |
| **R13**| 10 kΩ | `(D13, D17)` | Ladder bridge between Bit 2 and Bit 3 ($R$) |

#### External Wiring & Power Connections

* **`PB0` (LSB / Bit 0)** → Breadboard **`D1`** (from Nucleo CN8 pin 5 / A3)
* **`PB1` (Bit 1)**       → Breadboard **`D7`** (from Nucleo CN10 pin 24)
* **`PB2` (Bit 2)**       → Breadboard **`C11`** (from Nucleo CN10 pin 22)
* **`PB4` (MSB / Bit 3)** → Breadboard **`E15`** (from Nucleo CN10 pin 27 / D12)
* **Breadboard `A5`**     → **GND rail** (ladder ground termination)
* **Breadboard `A17`**    → **Buzzer (+ / Red wire)** (DAC analog output node $V_{OUT}$)
* **GND rail**            → **Buzzer (− / Black wire)**

#### Pin Selection Notes (Critical)
* ⚠️ **Why PB4 instead of PB3?**  
  On the Nucleo-F446RE, pin **PB3** is hard-wired to `SYS_JTDO-SWO` (Serial Wire Output debug trace) connected to the ST-LINK. It cannot be used as a reliable GPIO output. PB4 is completely free.
* ⚠️ **Why NOT PB6 for DAC?**  
  In Lab 13, the DAC used `PB3–PB6`. In Lab 15, **PB6 is dedicated to `I2C1_SCL` for the OLED display**. PB6 cannot be shared with the DAC ladder without corrupting both the I2C bus and the audio.

---

### Piezo Buzzer Acoustics & Frequency Tuning (`FGT-15T-6.0A1W40`)

The buzzer in the kit is a **ceramic piezo diaphragm** (`FGT-15T-6.0A1W40`):
* **Capacitance**: $C = 15{,}000\text{ pF} \pm 30\%$ ($15\text{ nF}$).
* **Resonant Frequency**: $6.0 \pm 0.5\text{ kHz}$.
* **Resonant Impedance**: $\le 300\,\Omega$.

#### Low-Pass Filtering by the R-2R Ladder
The $10\text{ k}\Omega$ output impedance of the R-2R ladder together with the $15\text{ nF}$ buzzer capacitance forms an **RC low-pass filter**:
$$f_{\text{cutoff}} = \frac{1}{2\pi R C} = \frac{1}{2\pi \times 10{,}000\,\Omega \times 15 \times 10^{-9}\,\text{F}} \approx \mathbf{1{,}061\text{ Hz}}$$

* Frequencies above $1\text{ kHz}$ are heavily attenuated by the $10\text{ k}\Omega$ series resistor.
* Frequencies between **$300\text{ Hz}$ and $800\text{ Hz}$** ($\text{half\_period} = 140\text{U}$ to $360\text{U}$) pass through with maximum voltage swing ($\approx 3\text{V}_{p-p}$), driving the diaphragm into clear, audible mechanical resonance.
* `Sound_Shoot()` implements an arcade downward laser frequency sweep from $140\text{U}$ ($\approx 780\text{ Hz}$) down to $360\text{U}$ ($\approx 280\text{ Hz}$), perfectly matching this acoustic window.

---

## Files to Copy into CubeIDE Project

Copy these files from `HAL/` into the corresponding CubeIDE directories:

| File | CubeIDE location |
|------|-----------------|
| `ssd1306_hal.c` | `Core/Src/` |
| `ssd1306_hal.h` | `Core/Inc/` |
| `dac_hal.c` | `Core/Src/` |
| `dac_hal.h` | `Core/Inc/` |
| `sound_hal.c` | `Core/Src/` |
| `sound_hal.h` | `Core/Inc/` |
| `game_types.h` | `Core/Inc/` |
| `../random.c` | `Core/Src/` |
| `../random.h` | `Core/Inc/` |

Also copy and adapt `SpaceInvaders_stm32.c` (see **Step 8** below).

---

## STM32CubeIDE IOC Configuration

### Step 1 — New Project

1. **File → New → STM32 Project**
2. Board: `NUCLEO-F446RE`
3. Project name: `Lab15_SpaceInvaders`
4. Language: **C**, firmware: **HAL**

---

### Step 2 — System Core → SYS

| Parameter | Value |
|-----------|-------|
| Debug | `Serial Wire` |

---

### Step 3 — Clock Configuration

Use **HSI 16 MHz**, no PLL (same as Lab 14):

| Field | Value |
|-------|-------|
| Input frequency | HSI 16 MHz |
| PLL | OFF |
| SYSCLK | 16 MHz |
| AHB Prescaler | /1 |
| APB1 Prescaler | /1 |
| APB2 Prescaler | /1 |

---

### Step 4 — Connectivity → I2C1 (OLED)

1. Left panel: **Connectivity → I2C1**
2. Mode = `I2C`
3. **Configuration → Parameter Settings**:
   - I2C Speed Mode = `Fast Mode`
   - I2C Clock Speed = `400000`
   - Duty Cycle = `Duty cycle Tlow/Thigh = 2`
4. Pinout view: default pins appear automatically:
   - `PB6` = `I2C1_SCL` (green)
   - `PB7` = `I2C1_SDA` (green)

> **To use PB8/PB9 instead (Arduino D15/D14):**  
> Right-click PB6 → *Reset State*; right-click PB7 → *Reset State*  
> Right-click PB8 → *I2C1_SCL*; right-click PB9 → *I2C1_SDA*

---

### Step 5 — Analog → ADC1 (Potentiometer)

1. Left panel: **Analog → ADC1**
2. IN0 = `IN0 (single-ended)` → PA0 turns green
3. **Configuration → Parameter Settings**:
   - Clock Prescaler = `PCLK2 divided by 2`
   - Resolution = `12 bits`
   - Data Alignment = `Right alignment`
   - Scan Conversion Mode = `Disabled`
   - Continuous Conversion = `Disabled`
   - Sampling Time (Channel 0) = `480 Cycles`

---

### Step 6 — GPIO (DAC outputs + fire buttons)

In the **Pinout view** (click each pin):

| Pin | Mode | Label |
|-----|------|-------|
| PB0 | `GPIO_Output` | `DAC_B0` |
| PB1 | `GPIO_Output` | `DAC_B1` |
| PB2 | `GPIO_Output` | `DAC_B2` |
| PB4 | `GPIO_Output` | `DAC_B3` |
| PC6 | `GPIO_EXTI6` | `BTN_MISSILE` |
| PC8 | `GPIO_EXTI8` | `BTN_LASER` |

For **PB0, PB1, PB2, PB4** (GPIO_Output):

> **Why PB4 instead of PB3?**  
> On the Nucleo-F446RE development board, pin PB3 is hard-wired to the Serial Wire Output (SWO) signal, which is used for debug tracing and ITM-based logging via the ST-LINK debugger. Attempting to use PB3 as a general-purpose GPIO output conflicts with the debugging interface and can cause trace communication failures or prevent proper debugging sessions. PB4, on the other hand, has no such hardware constraints on the Nucleo board and is fully available to act as the fourth bit of the R-2R DAC output.
- Output level: Low
- Mode: Output Push Pull
- Pull-up/Pull-down: No pull
- Maximum output speed: Low

For **PC6 and PC8** (GPIO_EXTI):
- GPIO mode: `External Interrupt Mode with Rising edge trigger detection`
- Pull-up/Pull-down: `Pull-down`

---

### Step 7 — NVIC (enable EXTI9_5 interrupt)

1. Left panel: **System Core → NVIC**
2. Find **EXTI line[9:5] interrupts**
3. Check the **Enabled** checkbox

> **Why priority 0 is fine here:**  
> The EXTI callback only sets a `volatile` flag (`missileFlag` / `laserFlag`) — it never
> calls HAL functions, so there is no risk of a SysTick re-entry conflict.  
> CubeIDE's default **Priority Group = 0** gives only one preemption level (0),
> which is why "1" does not appear in the dropdown. Leave both fields at **0**.

---

### Step 8 — Adapt SpaceInvaders_stm32.c

Copy `SpaceInvaders_stm32.c` to `Core/Src/SpaceInvaders.c` and apply these changes:

#### A. Change includes (top of file)

```c
// Remove:
#include "ssd1306.h"
#include "dac.h"
#include "sound_stm32.h"

// Add:
#include "stm32f4xx_hal.h"   /* for HAL_Delay */
#include "ssd1306_hal.h"
#include "dac_hal.h"
#include "sound_hal.h"
#include "game_types.h"      /* STyp struct */
```

#### B. Remove hardware-specific sections

Delete these entire sections from SpaceInvaders.c (they are now in gen.c/main.c):
- All `#define` register macros (RCC_AHB1ENR, GPIOA_MODER, ADC1_*, SYSCFG_*, EXTI_*, NVIC_*, STK_*)
- `ADC0_Init()` function
- `ADC0_In()` function
- `SysTick_Init_30Hz()` function
- `Switch_Init()` function
- `SysTick_Handler()` ISR
- `EXTI9_5_IRQHandler()` ISR
- `main()` function

#### C. Replace delay functions

```c
// Remove:
static void Delay100ms(uint32_t count) { ... }
static void Delay1ms(uint32_t msec) { ... }

// Replace all Delay100ms(n) calls with:
HAL_Delay(100U * n)

// Replace all Delay1ms(n) calls with:
HAL_Delay(n)
```

#### D. Critical Bug Fixes & Code Alignment

Apply these essential bug fixes to avoid screen freeze and logic glitches:

1. **Avoid I2C Bus Collision in `Game_Over()` and `Level_Check()` (Black Screen Root Cause)**:
   * ⚠️ **Do NOT call `SSD1306_Clear()`** inside `Game_Over()` or `Level_Check()` right before `SSD1306_DisplayBuffer()`.
   * `SSD1306_Clear()` transmits 1025 zero-bytes over I2C. Calling `SSD1306_DisplayBuffer()` immediately afterwards returns `HAL_BUSY` (bus still transmitting), causing the second call to fail silently and halting the MCU with a black screen.
   * **Fix**: Use `SSD1306_ClearBuffer()` (which only zeroes the RAM buffer) instead:
     ```c
     // In Game_Over() and Level_Check():
     // Replace:  SSD1306_Clear();
     // With:     SSD1306_ClearBuffer();
     ```

2. **Fix Unsigned Coordinate Underflow in `Enemy_Move()`**:
   * `Enemy[i].x` is unsigned. When moving left, adding `enemyDir` (-1 as `0xFFFFFFFF`) causes an underflow when `x` reaches 0.
   * **Fix**: Cast to signed `int32_t` for position calculation and boundary test:
     ```c
     Enemy[i].x = (uint32_t)((int32_t)Enemy[i].x + enemyDir);
     Enemy[i].maxpoint[0] = (uint32_t)((int32_t)Enemy[i].maxpoint[0] + enemyDir);
     ...
     if ((int32_t)Enemy[i].x <= 0) {
         Enemy[i].x = 0;
         enemyDir = 1 * (int32_t)currentLevel;
         enemyDrop = 2;
     }
     ```

3. **Fix Laser vs Bunker Index Typo & Missile Damage**:
   * In `Check_Damage()`, change `Laser[playMissile].life` to `Laser[playLaser].life` and extinguish the laser on impact: `Laser[playLaser].life = 0;`.
   * Ensure player missile deals 2 damage to enemies (`Enemy[i].life -= 2;`) while laser deals 1 damage (`Enemy[i].life -= 1;`).

---

### Step 9 — Fill in main.c USER CODE Sections

Use `main.c` / `gen.c` as the reference. Copy the content of each `USER CODE BEGIN/END` block into the corresponding block in CubeIDE's generated `main.c`:

| USER CODE block | What to add |
|-----------------|-------------|
| `BEGIN Includes` | Add driver includes + `game_types.h`, `random.h` |
| `BEGIN PV` | Extern game globals (`ADCdata`, `playerPosition`, `playerFlag`, `missileFlag`, etc.) |
| `BEGIN PFP` | Static helper prototypes: `ADC_Read()`, `Convert()` |
| `BEGIN 0` | `ADC_Read()` and `Convert()` helper implementations |
| `BEGIN 2` | `Sound_HAL_Init()`, `SSD1306_Init()`, `Random_Init(1U)`, world init, `Draw()` |
| `BEGIN 3` | **Entire while(1) game loop** (ADC 33ms sampling, attacks, movement, `Draw()`, `HAL_Delay(5U)`) |
| `BEGIN 4` | `HAL_GPIO_EXTI_Callback()` for PC6 (missile) & PC8 (laser) |

> ⚠️ **IMPORTANT CubeIDE Tag Placement**:
> Place the main game loop inside **`/* USER CODE BEGIN 3 */`** and **`/* USER CODE END 3 */`** (inside the `while (1)` body).
> If you place code between `USER CODE END WHILE` and `USER CODE BEGIN 3`, CubeIDE will wipe out your code whenever the IOC is saved or re-generated!

---

### Step 10 — Build and Flash

1. Right-click project → **Build Project** (`Ctrl+B`)
2. Connect Nucleo board via USB
3. **Run → Debug** (`F11`) or **Run → Run** (`Ctrl+F11`)
4. The OLED should show the Space Invaders game immediately.

---

## Architecture Notes

### SysTick / Timing

The bare-metal version uses a **SysTick ISR at 30 Hz** to read the ADC.  
The HAL version uses `HAL_GetTick()` (SysTick at 1 kHz, managed by HAL) to poll every **33 ms** in the main loop — no ISR override needed.

```
Bare-metal:  SysTick_Handler (30 Hz) → ADCdata, playerFlag
HAL:         Main loop checks HAL_GetTick() every 33 ms → same effect
```

### Button Interrupts

| Bare-metal | HAL |
|-----------|-----|
| `EXTI9_5_IRQHandler()` in SpaceInvaders.c | `HAL_GPIO_EXTI_Callback()` in main.c |
| Manual EXTI_PR clear | HAL clears pending flag automatically |

### Sound Timing

`tone_burst()` uses a spin-loop delay for pitch control — identical to the bare-metal version. The HAL SysTick IRQ (1 ms, ~16 000 cycles) introduces negligible frequency jitter for game sound effects.

### DAC vs HAL GPIO

`DAC_HAL_Out()` writes to `GPIOB->ODR` directly (one 32-bit write = 4 bits atomic).  
Using `HAL_GPIO_WritePin` four times would interleave sample bits and cause DAC glitches.
