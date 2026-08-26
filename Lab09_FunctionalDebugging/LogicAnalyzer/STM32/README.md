

### Signals to Capture (mirrors the Data[] array exactly)

| Channel | Signal | Pin | Connector | Active state |
|---|---|---|---|---|
| **CH1** | LED (PA5) | PA5 | CN10 pin 11 | HIGH = ON |
| **CH2** | SW2 (PC12) | PC12 | CN7 pin 2 | LOW = pressed |
| **CH3** | SW1 (PC13) | PC13 | CN7 pin 23 | LOW = pressed |
| **GND** | Ground | GND | CN10 pin 20 | — |



---

### Logic 2 Configuration

1. **Clear** all channels → enable only **`0`**, **`1`**, **`2`**
2. Sample rate: **`500 KS/s`**
3. Mode: **`Timer`** → **`5 s`**
4. Rename channels:
   - Channel 0 → `LED_PA5`
   - Channel 1 → `SW2_PC12`
   - Channel 2 → `SW1_PC13`

---



