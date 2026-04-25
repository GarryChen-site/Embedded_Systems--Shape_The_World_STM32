
typedef enum {
  S_EW_GREEN = 0,
  S_EW_YELLOW = 1,
  S_NS_GREEN = 2,
  S_NS_YELLOW = 3,
  S_WALK = 4,
  S_FLASH1 = 5,
  S_FLASH2 = 6,
  S_FLASH3 = 7,
  S_FLASH4 = 8,
  S_FLASH5 = 9,
  S_FLASH6 = 10,
} StateId;

typedef struct {
  uint32_t time_ms;
  uint8_t traffic;
  uint8_t walk;
  StateId next[8]; 
} State_t;

/* =========================================================================
 * FSM TABLE
 *
 * Sensor index (bit2=PED, bit1=NS_CAR, bit0=EW_CAR):
 *   next[0b000] = nobody    next[0b001] = EW only
 *   next[0b010] = NS only   next[0b011] = EW + NS
 *   next[0b100] = PED only  next[0b101] = PED + EW
 *   next[0b110] = PED + NS  next[0b111] = all three
 * ========================================================================= */

static const State_t FSM[11] = {
  { 200,  0x0C,   0x02, { S_EW_GREEN,  S_EW_GREEN,  S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW } }, /* S0: EW green  */
  { 300,  0x14,   0x02, { S_EW_YELLOW, S_EW_GREEN,  S_NS_GREEN,  S_NS_GREEN,  S_WALK,      S_WALK,      S_NS_GREEN,  S_NS_GREEN  } }, /* S1: EW yellow */
  { 200,  0x21,   0x02, { S_NS_GREEN,  S_NS_YELLOW, S_NS_GREEN,  S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW } }, /* S2: NS green  */
  { 300,  0x22,   0x02, { S_NS_YELLOW, S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN,  S_WALK,      S_EW_GREEN,  S_WALK,      S_WALK      } }, /* S3: NS yellow */
  { 200,  0x24,   0x08, { S_WALK,      S_FLASH1,    S_FLASH1,    S_FLASH1,    S_WALK,      S_FLASH1,    S_FLASH1,    S_FLASH1    } }, /* S4: WALK      */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH2,    S_FLASH2,    S_FLASH2,    S_WALK,      S_FLASH2,    S_FLASH2,    S_FLASH2    } }, /* S5: flash 1   */
  {  50,  0x24,   0x02, { S_WALK,      S_FLASH3,    S_FLASH3,    S_FLASH3,    S_WALK,      S_FLASH3,    S_FLASH3,    S_FLASH3    } }, /* S6: flash 2   */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH4,    S_FLASH4,    S_FLASH4,    S_WALK,      S_FLASH4,    S_FLASH4,    S_FLASH4    } }, /* S7: flash 3   */
  {  50,  0x24,   0x02, { S_WALK,      S_FLASH5,    S_FLASH5,    S_FLASH5,    S_WALK,      S_FLASH5,    S_FLASH5,    S_FLASH5    } }, /* S8: flash 4   */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH6,    S_FLASH6,    S_FLASH6,    S_WALK,      S_FLASH6,    S_FLASH6,    S_FLASH6    } }, /* S9: flash 5   */
  {  50,  0x24,   0x02, { S_FLASH1,    S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN,  S_WALK,      S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN  } }, /* S10: flash 6  */
};