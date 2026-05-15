
#include "piano.h"
#include "sound.h"

int main(void) {

  uint8_t key;

  Sound_Init();
  Piano_Init();

  while (1) {

    key = Piano_In();

    switch (key) {
    case SILENCE:
      Sound_Off();
    case DO:
      Sound_Tone(DO_PERIOD);
      break;
    case RE:
      Sound_Tone(RE_PERIOD);
      break;
    case MI:
      Sound_Tone(MI_PERIOD);
      break;
    case SOL:
      Sound_Tone(SOL_PERIOD);
      break;
    default:
      Sound_Off();
      break;
    }
  }
}