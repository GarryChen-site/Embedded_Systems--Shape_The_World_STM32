
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
  //  bit5=0 EW_RED  OFF    bit4=0 EW_YELLOW OFF    bit3=1 EW_GREEN  ON ✓
  //  bit2=1 NS_RED  ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN  OFF
  // -------------------------------------------------------------------
  // nobody -> stay EW green; EW only -> stay EW green; NS only -> go EW yellow; EW+NS -> go EW yellow; 
  // PED only -> go EW yellow; PED+EW -> go EW yellow; PED+NS -> go EW yellow; all three -> go EW yellow
  { 200,  0x0C,   0x02, { S_EW_GREEN,  S_EW_GREEN,  S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW, S_EW_YELLOW } }, /* S0: EW green  */
  
  // bit5=0 EW_RED OFF    bit4=1 EW_YELLOW ON ✓    bit3=0 EW_GREEN OFF
  // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF      bit0=0 NS_GREEN OFF
  // -------------------------------------------------------------------
  // nobody -> stay EW yellow; EW only -> go EW green; NS only -> go NS green; EW+NS -> go NS green; 
  // PED only -> go WALK; PED+EW -> go WALK; PED+NS -> go NS green; all three -> go NS green
  { 300,  0x14,   0x02, { S_EW_YELLOW, S_EW_GREEN,  S_NS_GREEN,  S_NS_GREEN,  S_WALK,      S_WALK,      S_NS_GREEN,  S_NS_GREEN  } }, /* S1: EW yellow */
  
  // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
  // bit2=0 NS_RED OFF    bit1=0 NS_YELLOW OFF    bit0=1 NS_GREEN ON ✓
  // -------------------------------------------------------------------
  // nobody -> stay NS green; EW only -> go NS yellow; NS only -> stay NS green; EW+NS -> go NS yellow; 
  // PED only -> go NS yellow; PED+EW -> go NS yellow; PED+NS -> go NS yellow; all three -> go NS yellow
  { 200,  0x21,   0x02, { S_NS_GREEN,  S_NS_YELLOW, S_NS_GREEN,  S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW, S_NS_YELLOW } }, /* S2: NS green  */
  
  // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
  // bit2=0 NS_RED OFF    bit1=1 NS_YELLOW ON ✓   bit0=0 NS_GREEN OFF
  // -------------------------------------------------------------------
  // nobody -> stay NS yellow; EW only -> go EW green; NS only -> go NS green; EW+NS -> go EW green; 
  // PED only -> go WALK; PED+EW -> go EW green; PED+NS -> go WALK; all three -> go WALK
  { 300,  0x22,   0x02, { S_NS_YELLOW, S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN,  S_WALK,      S_EW_GREEN,  S_WALK,      S_WALK      } }, /* S3: NS yellow */
  
  // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
  // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN OFF
  // -------------------------------------------------------------------
  // a car arriving is the only trigger to end the WALK state.
  // nobody -> stay WALK; EW only -> go FLASH 1; NS only -> go FLASH 1; EW+NS -> go FLASH 1; 
  // PED only -> stay WALK; PED+EW -> go FLASH 1; PED+NS -> go FLASH 1; all three -> go FLASH 1
  { 200,  0x24,   0x08, { S_WALK,      S_FLASH1,    S_FLASH1,    S_FLASH1,    S_WALK,      S_FLASH1,    S_FLASH1,    S_FLASH1    } }, /* S4: WALK      */
  
  // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
  // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN OFF
  // -------------------------------------------------------------------
  // during the flash chain, the only escape back to WALK is when no car is detected. Any car keeps the flash advancing forward
  // nobody -> go WALK; EW only -> go FLASH 2; NS only -> go FLASH 2; EW+NS -> go FLASH 2; 
  // PED only -> go WALK; PED+EW -> go FLASH 2; PED+NS -> go FLASH 2; all three -> go FLASH 2
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH2,    S_FLASH2,    S_FLASH2,    S_WALK,      S_FLASH2,    S_FLASH2,    S_FLASH2    } }, /* S5: flash 1   */
  {  50,  0x24,   0x02, { S_WALK,      S_FLASH3,    S_FLASH3,    S_FLASH3,    S_WALK,      S_FLASH3,    S_FLASH3,    S_FLASH3    } }, /* S6: flash 2   */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH4,    S_FLASH4,    S_FLASH4,    S_WALK,      S_FLASH4,    S_FLASH4,    S_FLASH4    } }, /* S7: flash 3   */
  {  50,  0x24,   0x02, { S_WALK,      S_FLASH5,    S_FLASH5,    S_FLASH5,    S_WALK,      S_FLASH5,    S_FLASH5,    S_FLASH5    } }, /* S8: flash 4   */
  {  50,  0x24,   0x00, { S_WALK,      S_FLASH6,    S_FLASH6,    S_FLASH6,    S_WALK,      S_FLASH6,    S_FLASH6,    S_FLASH6    } }, /* S9: flash 5   */
  
  // bit5=1 EW_RED ON ✓   bit4=0 EW_YELLOW OFF    bit3=0 EW_GREEN OFF
  // bit2=1 NS_RED ON ✓   bit1=0 NS_YELLOW OFF    bit0=0 NS_GREEN OFF
  // -------------------------------------------------------------------  
  // a car must be waiting to break out of the walk/flash cycle. 
  // If no cars: with pedestrian → WALK again; without pedestrian → restart the flash loop at FLASH1.
  // nobody -> restart FLASH 1; EW only -> go EW GREEN; NS only -> go NS GREEN; EW+NS -> go EW GREEN; 
  // PED only -> go WALK; PED+EW -> go EW GREEN; PED+NS -> go NS GREEN; all three -> go EW GREEN
  {  50,  0x24,   0x02, { S_FLASH1,    S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN,  S_WALK,      S_EW_GREEN,  S_NS_GREEN,  S_EW_GREEN  } }, /* S10: flash 6  */
};

static void apply_traffic(uint8_t traffic, uint8_t walk);
static uint8_t read_sensors(void);


int main(void) {
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();

  uint8_t state = 0;

  while(1) {
    // A Moore FSM because outputs are associated with states, not transitions.
    apply_traffic(FSM[state].traffic, FSM[state].walk);
    HAL_Delay(FSM[state].time_ms);
    state = FSM[state].next[read_sensors()];
  }
}