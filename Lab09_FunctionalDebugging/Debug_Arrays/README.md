

```
bit 2: SW1/PC13  (1=released, 0=pressed)
bit 1: LED/PA5   (1=ON,       0=OFF)
bit 0: SW2/PC12  (1=released, 0=pressed)
```

---

## Decoding Data[]

| Index | Value | Binary | SW1 | LED | SW2 | Meaning |
|---|---|---|---|---|---|---|
| **[0]** | **5** | `101` | released | **OFF** | released | ← Initial state: nothing pressed |
| **[1]** | **3** | `011` | **pressed** | **ON** | released | SW1 pressed → LED turned ON |
| **[2]** | **1** | `001` | pressed | **OFF** | released | LED toggled OFF (50ms) |
| **[3]** | **3** | `011` | pressed | **ON** | released | LED toggled ON (50ms) |
| **[4]** | **1** | `001` | pressed | OFF | released | LED toggled OFF |
| … | … | … | pressed | toggling… | released | 10 Hz blink continues |
| **[13]** | **5** | `101` | **released** | **OFF** | released | ← SW1 released, LED stops |
| [14]+ | **0** | `000` | — | — | — | Not yet written (buffer empty) |

---

## What the Pattern Proves 

```
Data: 5 → 3 → 1 → 3 → 1 → 3 → 1 → 3 → 1 → 3 → 1 → 3 → 1 → 5
```

- **Alternating `3` and `1`** = LED perfectly toggling at 10 Hz 
- **No two adjacent values are equal** = only changes recorded 
- **Starts and ends with `5`** = both switches released 
- **`Data[14]` onward = `0`** = buffer not yet filled (normal) 

The recording perfectly captured: **press SW1 → LED blinks 6 times → release SW1 → LED stops**.