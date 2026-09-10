

## Verify each connection against the schematic

| Resistor | Schematic role | Breadboard holes | Connects |
|----------|---------------|-----------------|---------|
| **R1** | D0 → midA | **(A3) ↔ (A4)** | PB3 wire to intermediate row |
| **R2** | midA → N0 | **(B4) ↔ (B5)** | intermediate to N0 node |
| **R3** | N0 → midB (termination) | **(C5) ↔ (C6)** | N0 downward |
| **R4** | midB → GND | **(D6) ↔ (D7)** | intermediate to GND row |
| **wire** | D7 → GND rail | **(D7) → blue rail** | connect to GND |
| **R5** | N0 → N1 (rail) | **(E5) ↔ (E11)** | spans 6 rows |
| **R6** | D1 → midC | **(A9) ↔ (A10)** | PB4 wire to intermediate |
| **R7** | midC → N1/VOUT | **(B10) ↔ (B11)** | intermediate to VOUT |
| **wire** | VOUT → PA0 | **(C11) → PA0** | ADC measurement |



