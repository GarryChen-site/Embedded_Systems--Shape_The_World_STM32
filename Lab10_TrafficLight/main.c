
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
  uint32_t time_ms;  // HOW LONG to stay in this state
  uint8_t traffic;   // WHAT the 6 traffic LEDs show
  uint8_t walk;      // WHAT the 2 walk LEDs show
  StateId next[8];   // WHERE to go next, based on sensors. Three binary sensors, each can be 0 or 1, 8 possible combinations 
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

 /*
 * 3 states(EW greem, NS green, Walk) + 2 yellow states + 6 flash states = 11 states 
 * Green phase time = 200ms
 * Yellow phase time = 300ms
 * Walk phase time = 200ms
 * Flash phase time = 50ms
 * 3 Flashes need 6 States (On, Off, On, Off, On, Off)
 */

static const State_t FSM[11] = {
  // nobody -> stay EW green; EW only -> stay EW green; NS only -> go EW yellow; EW+NS -> go EW yellow; 
  // PED only -> go EW yellow; PED+EW -> go EW yellow; PED+NS -> go EW yellow; all three -> go EW yellow
  { 200,  0x0C,   0x02, { S_EW_GREEN,  S_EW_GREEN,  S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW } }, /* S0: EW green  */
  
  // nobody -> stay EW yellow; EW only -> go EW green; NS only -> go NS green; EW+NS -> go NS green; 
  // PED only -> go WALK; PED+EW -> go WALK; PED+NS -> go NS green; all three -> go NS green
  { 300,  0x14,   0x02, { S_EW_YELLOW, S_EW_GREEN,  S_NS_GREEN,  S_NS_GREEN,  S_WALK,      S_WALK,      S_NS_GREEN,  S_NS_GREEN  } }, /* S1: EW yellow */
  
  // nobody -> stay NS green; EW only -> go NS yellow; NS only -> stay NS green; EW+NS -> go NS yellow; 
  // PED only -> go NS yellow; PED+EW -> go NS yellow; PED+NS -> go NS yellow; all three -> go NS yellow
  { 200,  0x21,   0x02, { S_NS_GREEN,  S_NS_YELLOW, S_NS_GREEN,  S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW } }, /* S2: NS green  */
  
  // nobody -> stay NS yellow; EW only -> go EW green; NS only -> go NS green; EW+NS -> go EW green; 
  // PED only -> go WALK; PED+EW -> go EW green; PED+NS -> go WALK; all three -> go WALK
  { 300,  0x22,   0x02, { S_NS_YELLOW, S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN,  S_WALK,      S_EW_GREEN,  S_WALK,      S_WALK      } }, /* S3: NS yellow */
  
  { 200,  0x24,   0x08, { S_WALK,      S_FLASH1,    S_FLASH1,    S_FLASH1,    S_WALK,      S_FLASH1,    S_FLASH1,    S_FLASH1    } }, /* S4: WALK      */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH2,    S_FLASH2,    S_FLASH2,    S_WALK,      S_FLASH2,    S_FLASH2,    S_FLASH2    } }, /* S5: flash 1   */
  {  50,  0x24,   0x02, { S_WALK,      S_FLASH3,    S_FLASH3,    S_FLASH3,    S_WALK,      S_FLASH3,    S_FLASH3,    S_FLASH3    } }, /* S6: flash 2   */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH4,    S_FLASH4,    S_FLASH4,    S_WALK,      S_FLASH4,    S_FLASH4,    S_FLASH4    } }, /* S7: flash 3   */
  {  50,  0x24,   0x02, { S_WALK,      S_FLASH5,    S_FLASH5,    S_FLASH5,    S_WALK,      S_FLASH5,    S_FLASH5,    S_FLASH5    } }, /* S8: flash 4   */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH6,    S_FLASH6,    S_FLASH6,    S_WALK,      S_FLASH6,    S_FLASH6,    S_FLASH6    } }, /* S9: flash 5   */
  {  50,  0x24,   0x02, { S_FLASH1,    S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN,  S_WALK,      S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN  } }, /* S10: flash 6  */
};