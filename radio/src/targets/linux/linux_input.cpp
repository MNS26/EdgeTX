#include "board.h"
#include "hal/key_driver.h"
#include "hal/adc_driver.h"

#if defined(HARDWARE_REAL_INPUTS)
#include "mcp23017_driver.h"
#endif

extern const etx_hal_adc_driver_t pi_adc_driver;

const etx_hal_adc_driver_t* getDefaultAdcDriver()
{
  return &pi_adc_driver;
}

uint32_t readKeys()
{
#if defined(HARDWARE_REAL_INPUTS)
  // TODO: read MCP23017 GPIO registers for key states
  // Pin mapping depends on wiring — return 0 (no keys pressed) for now
  return 0;
#else
  uint32_t result = 0;
  for (int i = 0; i < MAX_KEYS; i++) {
    if (keysStates[i])
      result |= 1 << i;
  }
  return result;
#endif
}

uint32_t readTrims()
{
#if defined(HARDWARE_REAL_INPUTS)
  // TODO: read MCP23017 GPIO registers for trim states
  // Pin mapping depends on wiring — return 0 (no trims pressed) for now
  return 0;
#else
  uint32_t trims = 0;
  for (int i = 0; i < keysGetMaxTrims() * 2; i++) {
    if (trimsStates[i])
      trims |= 1 << i;
  }
  return trims;
#endif
}
