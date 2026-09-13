// measure.h — Lab 14 ADC, STM32 F446RE port
//
// Trimmer potentiometer: 270° mechanical, 240° adjustment angle.
// Output unit: 0.1° (tenths of a degree), range 0–2400.
#ifndef MEASURE_H
#define MEASURE_H
#include <stdint.h>


// Potentiometer physical parameters
#define POT_ADJUSTMENT_ANGLE  240   // usable electrical sweep in degrees
#define POT_TENTHS_MAX       2400   // max output in 0.1° units (240.0°)

// Convert — map 12-bit ADC sample to angle in 0.1° units
// Input:  sample  0–4095
// Output: angle   0–2400  (0.0° to 240.0°, resolution 0.1°)
uint32_t Convert(uint32_t sample);

// ConvertToString — format angle into "DDD.D deg" ASCII string
// Input:  angle  0–2400 (0.1° units)
// Output: fills global String[] with null-terminated display string
void ConvertToString(uint32_t angle);

// String buffer for formatted output ("DDD.D deg\0")
extern char String[12];

#endif