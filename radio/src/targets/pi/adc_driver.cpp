#include "board.h"
#include "hal/adc_driver.h"
#include "myeeprom.h"
#include "translations/translations.h"
#include "hal_adc_inputs.inc"

#include "ads1115_driver.h"

#define I2C_BUS "/dev/i2c-1"

#define ADS1115_ADDR_1 0x48
#define ADS1115_ADDR_2 0x49

struct ads1115_channel {
  uint8_t addr;
  uint8_t mux;
};

static const struct ads1115_channel ch_map[] = {
  { ADS1115_ADDR_1, 0 },  // 0: LH
  { ADS1115_ADDR_1, 1 },  // 1: LV
  { ADS1115_ADDR_1, 2 },  // 2: RV
  { ADS1115_ADDR_1, 3 },  // 3: RH
  { ADS1115_ADDR_2, 0 },  // 4: P1
  { ADS1115_ADDR_2, 1 },  // 5: P2
  { ADS1115_ADDR_2, 2 },  // 6: SL1
  { ADS1115_ADDR_2, 3 },  // 7: SL2
};
#define PI_ADC_ACTIVE_CHANNELS 8

static int ads1115_fd = -1;

static bool pi_adc_init()
{
  ads1115_fd = ads1115_open(I2C_BUS);
  return true;
}

static bool pi_start_conversion()
{
  for (int i = 0; i < PI_ADC_ACTIVE_CHANNELS; i++) {
    int16_t raw = ads1115_read(ads1115_fd, ch_map[i].addr, ch_map[i].mux);
    setAnalogValue(i, ads1115_scale(raw));
  }

  int max_input = adcGetMaxInputs(ADC_INPUT_ALL);
  for (int i = PI_ADC_ACTIVE_CHANNELS; i < max_input; i++) {
    setAnalogValue(i, 2048);
  }

  return true;
}

void enableVBatBridge() {}
void disableVBatBridge() {}
bool isVBatBridgeEnabled() { return false; }

uint16_t getLuxSensorValue()
{
  if (adcGetMaxInputs(ADC_INPUT_LUX) < 1) return 0;
  return anaIn(adcGetInputOffset(ADC_INPUT_LUX));
}

uint16_t getRTCBatteryVoltage()
{
  return 300;
}

extern const etx_hal_adc_driver_t pi_adc_driver = {
  .inputs = _hal_inputs,
  .default_pots_cfg = _pot_default_config,
  .init = pi_adc_init,
  .start_conversion = pi_start_conversion,
  .wait_completion = nullptr,
};
