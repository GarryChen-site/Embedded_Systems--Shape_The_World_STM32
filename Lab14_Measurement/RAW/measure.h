#ifndef MEASURE_H
#define MEASURE_H
#include <stdint.h>


#define POT_ADJUSTMENT_ANGLE   240
#define POT_TENTHS_MAX         2400

uint32_t Convert(uint32_t sample);

void ConvertToString(uint32_t angle);

extern char String[12];

#endif