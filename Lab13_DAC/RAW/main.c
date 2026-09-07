
#include "piano.h"
#include "sound.h"

int main(void) {

  uint8_t key;

  Sound_Init(); // initialize SysTick + DAC (calls DAC_Init internally)
  Piano_Init(); // initialize 4 key inputs
  // SysTick interrupts are already enabled inside Sound_Init (STK_CTRL = 0x07)

  while (1) {

    key = Piano_In();

    switch (key) {
    case SILENCE:
      Sound_Off();
      break;
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