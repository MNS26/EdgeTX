#include "board.h"

#include "hal/switch_driver.h"
#include "definitions.h"
#include "myeeprom.h"
#include "switches.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "mcp23017_driver.h"

#define MCP23017_ADDR_KEYS      0x20
#define MCP23017_ADDR_TRIMS     0x21
#define MCP23017_ADDR_SWITCHES  0x22
#define I2C_BUS                 "/dev/i2c-1"

static int mcp23017_fd = -1;

static int mcp23017_init_bus()
{
  mcp23017_fd = mcp23017_open(I2C_BUS);
  if (mcp23017_fd < 0) return -1;

  mcp23017_set_dir(mcp23017_fd, MCP23017_ADDR_KEYS, 0xFFFF);
  mcp23017_set_dir(mcp23017_fd, MCP23017_ADDR_TRIMS, 0xFFFF);
  mcp23017_set_dir(mcp23017_fd, MCP23017_ADDR_SWITCHES, 0xFFFF);

  mcp23017_set_pullups(mcp23017_fd, MCP23017_ADDR_KEYS, 0xFFFF);
  mcp23017_set_pullups(mcp23017_fd, MCP23017_ADDR_TRIMS, 0xFFFF);
  mcp23017_set_pullups(mcp23017_fd, MCP23017_ADDR_SWITCHES, 0xFFFF);

  return 0;
}

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

#if defined(RADIO_GX12)
void _poll_switches() {}
#endif

void simuSetSwitch(uint8_t swtch, int8_t state)
{
  assert(swtch < switchGetMaxAllSwitches());
  switchesStates[swtch] = state;
}

void boardInitSwitches()
{
  memset(switchesStates, -1, sizeof(switchesStates));
  mcp23017_init_bus();
}

SwitchHwPos boardSwitchGetPosition(uint8_t idx)
{
  // TODO: map switch index to MCP23017 address + pin, decode 2POS/3POS
  // This depends on physical wiring

  if (switchesStates[idx] < 0)
    return SWITCH_HW_UP;
  else if (switchesStates[idx] == 0)
    return SWITCH_HW_MID;
  else
    return SWITCH_HW_DOWN;
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
