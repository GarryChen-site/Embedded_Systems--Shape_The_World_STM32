// measure.c — Lab 14 ADC, STM32 F446RE port
//
// Replaces the TM4C's Convert() and UART_ConvertDistance() functions.
// Adapted for a rotary trimming potentiometer instead of a linear slide pot.
//
// ORIGINAL LAB14 (TM4C, slide pot):
//   - Physical quantity: distance (0–2 cm)
//   - Output unit: 0.001 cm  → values 0–2000
//   - Formula: Distance = sample / 4095.0 × Size × 1000
//   - Display: "D.DDD cm"  (1 digit before decimal, 3 after)
//
// THIS PORT (STM32, square trimmer 270° / 240° adjustment):
//   - Physical quantity: angle (0°–240°)
//   - Output unit: 0.1°      → values 0–2400
//   - Formula: Angle = sample × 2400 / 4095
//   - Display: "DDD.D deg"  (3 digits before decimal, 1 after)
//
// WHY 240° AND NOT 270°?
//   The trimmer's MECHANICAL angle (270°) is the full physical travel.
//   The ADJUSTMENT angle (240°) is the ELECTRICAL range — the portion where
//   the wiper actually moves between GND and VCC.  The remaining 30° is a
//   dead-zone gap at the CW end where the pot has no more electrical travel
//   but the shaft can still rotate mechanically.
//   We calibrate against the 240° electrical range, not the 270° shaft travel.
//
// CONVERSION FORMULA:
//
//   ADC sample: 0 to 4095  (12-bit, right-aligned)
//   Angle (0.1°): 0 to 2400
//
//   angle_tenths = sample × 2400 / 4095
//
//   Integer arithmetic (no floating point needed on bare metal):
//     - Multiply first to avoid truncation: sample × 2400 fits in 32 bits
//       (max = 4095 × 2400 = 9,828,000 < 2^32)
//     - Divide by 4095
//
// EXAMPLES:
//   sample=0    → 0    → "0.0 deg"   (fully CCW)
//   sample=2047 → 1200 → "120.0 deg" (midpoint ≈ 120°)
//   sample=4095 → 2400 → "240.0 deg" (fully CW)

#include "measure.h"
#include <stdint.h>

char String[12]; // "DDD.D deg\0" = 10 chars + safety

// -------------------------------------------------------------------------
// Convert — 12-bit ADC sample → angle in 0.1° units
// -------------------------------------------------------------------------
uint32_t Convert(uint32_t sample) {
  // sample × 2400 / 4095
  // Multiply before divide to preserve integer precision.
  // Max: 4095 × 2400 = 9,828,000  (fits in uint32_t)
  return (sample * (uint32_t)POT_TENTHS_MAX) / 4095U;
}

// -------------------------------------------------------------------------
// ConvertToString — format angle (0–2400) into "DDD.D deg\0"
//
// Output format: always 3 integer digits + '.' + 1 decimal + " deg"
//   e.g.   0 → "  0.0 deg"
//        240 → " 24.0 deg"
//       1234 → "123.4 deg"
//       2400 → "240.0 deg"
//      >2400 → "***.* deg"  (overflow guard)
// -------------------------------------------------------------------------
void ConvertToString(uint32_t angle) {
  if (angle > POT_TENTHS_MAX) {
    // Overflow: display asterisks
    String[0] = '*';
    String[1] = '*';
    String[2] = '*';
    String[3] = '.';
    String[4] = '*';
  } else {
    // angle is 0–2400, representing 0.0°–240.0°
    // Integer part: angle / 10   (0–240)
    // Fractional part: angle % 10 (0–9)
    uint32_t integer_part = angle / 10U;
    uint32_t decimal_digit = angle % 10U;

    // Fill 3 integer digits, right-to-left
    String[2] = (char)('0' + (integer_part % 10));
    integer_part /= 10;
    String[1] = (char)('0' + (integer_part % 10));
    integer_part /= 10;
    String[0] = (char)('0' + (integer_part % 10));

    // Decimal point and fractional digit
    String[3] = '.';
    String[4] = (char)('0' + decimal_digit);
  }

  // Suffix and null termination
  String[5] = ' ';
  String[6] = 'd';
  String[7] = 'e';
  String[8] = 'g';
  String[9] = '\0';
  String[10] = '\0';
}