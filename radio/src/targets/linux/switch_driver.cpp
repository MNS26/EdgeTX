#include "board.h"

#include "hal/switch_driver.h"
#include "definitions.h"
#include "myeeprom.h"
#include "switches.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "mcp23017_driver.h"

struct hw_switch_def {
  const char*   name;
  SwitchHwType  type;
  SwitchConfig  defaultType;
#if defined(FUNCTION_SWITCHES)
  bool          isCustomSwitch;
  uint8_t       customSwitchIdx;
#endif
};

#include "simu_switches.inc"

int8_t switchesStates[MAX_SWITCHES];

// MCP23017 pin mapping for switches on chip at 0x22
// Sequential: 3POS uses 2 pins (hi, lo), 2POS uses 1 pin (hi)
struct sw_pin {
  uint8_t hi;
  uint8_t lo;   // 0xFF if 2POS
};

static const sw_pin switch_pins[] = {
  { 0,  1   },  // SA: 3POS
  { 2,  3   },  // SB: 3POS
  { 4,  5   },  // SC: 3POS
  { 6,  7   },  // SD: 3POS
  { 8,  9   },  // SE: 3POS
  { 10, 255 },  // SF: 2POS
  { 11, 12  },  // SG: 3POS
  { 13, 255 },  // SH: 2POS
  { 14, 255 },  // SI: 2POS
  { 15, 255 },  // SJ: 2POS
};

void boardInitSwitches()
{
  memset(switchesStates, -1, sizeof(switchesStates));
  mcp23017_init_bus();
}

SwitchHwPos boardSwitchGetPosition(uint8_t idx)
{
  if (idx >= n_switches) return SWITCH_HW_UP;

  int fd = mcp23017_get_bus_fd();
  if (fd < 0) {
    //switchesStates[idx] = (rand()%2)-1;
    // No hardware: return cached software state (e.g. from simu)
    switch (switchesStates[idx]) {
      case -1:
        return SWITCH_HW_UP;
      case 0:
        return SWITCH_HW_MID;
      case 1:
        return SWITCH_HW_DOWN;
      default:
        return SWITCH_HW_MID;
    }
  }

  uint16_t gpio = mcp23017_read_gpio(fd, MCP23017_ADDR_SWITCHES);
  const sw_pin& p = switch_pins[idx];
  SwitchHwPos ret = SWITCH_HW_UP;

  if (p.lo == 0xFF) {
    // 2POS: hi pin HIGH = UP, LOW = DOWN
    if (!(gpio & (1 << p.hi)))
      ret = SWITCH_HW_DOWN;
  } else {
    // 3POS: decode from hi/lo pin states
    uint8_t pins_state = (p.lo) | (p.hi<<1);
    switch (pins_state) {
      case 0b11: // 1 & 2 high
        ret = SWITCH_HW_MID;
        break;
      case 0b10: // 1 high, 2 low
        ret = SWITCH_HW_UP;
        break;
      case 0b01: // 1 low, 2 high
        ret = SWITCH_HW_DOWN;
        break;
      default:
        ret = SWITCH_HW_UP;
        break;
    }

//    bool hi = gpio & (1 << p.hi);
//    bool lo = gpio & (1 << p.lo);
//    if (hi && lo)
//      ret = SWITCH_HW_MID;
//    else if (!hi && lo)
//      ret = SWITCH_HW_DOWN;
//    // else UP (default)
  }

//  if (p.inverted) {
//    if (ret == SWITCH_HW_UP)
//      ret = SWITCH_HW_DOWN;
//    else if (ret == SWITCH_HW_DOWN)
//      ret = SWITCH_HW_UP;
//  }

  return ret;
}

const char* boardSwitchGetName(uint8_t idx)
{
  return _switch_defs[idx].name;
}

SwitchHwType boardSwitchGetType(uint8_t idx)
{
  return _switch_defs[idx].type;
}

uint8_t boardGetMaxSwitches() { return n_switches; }

SwitchConfig boardSwitchGetDefaultConfig(uint8_t idx) { return _switch_defs[idx].defaultType; }

#if defined(FUNCTION_SWITCHES)
bool boardIsCustomSwitch(uint8_t idx) { return (idx < n_switches) ? _switch_defs[idx].isCustomSwitch : false; }
uint8_t boardGetCustomSwitchIdx(uint8_t idx) { return _switch_defs[idx].customSwitchIdx; }
#endif

#if !defined(COLORLCD)
switch_display_pos_t switchGetDisplayPosition(uint8_t idx)
{
  if (idx >= DIM(_switch_display)) return {0, 0};
  return _switch_display[idx];
}
#endif
