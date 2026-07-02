#include "board.h"
#include "hal/key_driver.h"
#include "hal/adc_driver.h"

extern const etx_hal_adc_driver_t simu_adc_driver;

const etx_hal_adc_driver_t* getDefaultAdcDriver()
{
  return &simu_adc_driver;
}

extern bool keysStates[];
extern bool trimsStates[];

uint32_t readKeys()
{
  uint32_t result = 0;
  for (int i = 0; i < MAX_KEYS; i++) {
    if (keysStates[i])
      result |= 1 << i;
  }
  return result;
}

uint32_t readTrims()
{
  uint32_t trims = 0;
  for (int i = 0; i < keysGetMaxTrims() * 2; i++) {
    if (trimsStates[i])
      trims |= 1 << i;
  }
  return trims;
}
