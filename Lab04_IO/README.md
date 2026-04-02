# Lab 4 IO — STM32 F446RE Port
**Course**: Embedded Systems: Shape The World (UTAustinX UT.6.x)  
**Original hardware**: TI TM4C123GX (Tiva LaunchPad)  
**Ported to**: STM32 F446RE Nucleo board  

---


## Original vs Adapted Behavior

| Switch State | TM4C LED | STM32 LD2 (PA5) |
|---|---|---|
| Both SW1 + SW2 pressed | 🔵 Blue (PF2) | **Solid ON** |
| SW1 only pressed | 🔴 Red (PF1) | **Fast blink** (200 ms) |
| SW2 only pressed | 🟢 Green (PF3) | **Slow blink** (800 ms) |
| Neither pressed | ⚫ Off | **Solid OFF** |

---