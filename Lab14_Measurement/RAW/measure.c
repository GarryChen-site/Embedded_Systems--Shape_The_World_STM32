
#include "measure.h"
#include <stdint.h>

uint32_t Convert(uint32_t sample) {
  return (sample * (uint32_t)POT_TENTHS_MAX) / 4095;
}

void ConvertToString(uint32_t angle) {

  if (angle > POT_TENTHS_MAX) {
    String[0] = '*';
    String[1] = '*';
    String[2] = '*';
    String[3] = '.';
    String[4] = '*';
  } else {
    uint32_t integer_part = angle / 10;
    uint32_t decimal_part = angle % 10;

    String[2] = (char)('0' + (integer_part % 10));
    integer_part /= 10;

    String[1] = (char)('0' + (integer_part % 10));
    integer_part /= 10;

    String[0] = (char)('0' + integer_part);

    String[3] = '.';
    String[4] = (char)('0' + decimal_part);
  }

  String[5] = ' ';
  String[6] = 'd';
  String[7] = 'e';
  String[8] = 'g';
  String[9] = '\0';
  String[10] = '\0';
}