#include "board.h"
#include "hal/key_driver.h"
#include "hal/adc_driver.h"
#include <stdlib.h>
#if defined(HARDWARE_REAL_INPUTS)
#include "mcp23017_driver.h"
#endif

extern const etx_hal_adc_driver_t linux_adc_driver;

const etx_hal_adc_driver_t* getDefaultAdcDriver()
{
  return &linux_adc_driver;
}

uint32_t readKeys()
{
#if defined(HARDWARE_REAL_INPUTS)
  // Get i2c bus filedescriptor
  int fd = mcp23017_get_bus_fd();
  if (fd < 0) return 0;

  uint16_t gpio = mcp23017_read_gpio(fd, MCP23017_ADDR_KEYS);
  // All keys are active-low, so inverted GPIO gives pressed state
  gpio = ~gpio;

  uint32_t result = 0;
  if (gpio & (1 << 0)) result |= (1 << KEY_EXIT);
  if (gpio & (1 << 1)) result |= (1 << KEY_ENTER);
  if (gpio & (1 << 2)) result |= (1 << KEY_PAGEUP);
  if (gpio & (1 << 3)) result |= (1 << KEY_PAGEDN);
  if (gpio & (1 << 4)) result |= (1 << KEY_MODEL);
  if (gpio & (1 << 5)) result |= (1 << KEY_TELE);
  if (gpio & (1 << 6)) result |= (1 << KEY_SYS);
  return result;
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
  // Get i2c bus filedescriptor
  int fd = mcp23017_get_bus_fd();
  if (fd < 0) return 0;

  uint16_t gpio = mcp23017_read_gpio(fd, MCP23017_ADDR_TRIMS);
  // Active-low: invert to get pressed state
  return ~gpio & 0xFFF;  // only 12 trim bits
#else
  uint32_t trims = 0;
  for (int i = 0; i < keysGetMaxTrims() * 2; i++) {
    if (trimsStates[i])
      trims |= 1 << i;
  }
  return trims;
#endif
}
