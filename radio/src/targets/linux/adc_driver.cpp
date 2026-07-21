#include "board.h"
#include "hal/adc_driver.h"
#include "myeeprom.h"
#include "translations/translations.h"
#include "hal_adc_inputs.inc"

#include "ads1115_driver.h"


#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])

// array of i2c addresses
static uint8_t addrs[] = {ADS1115_ADDR_1, ADS1115_ADDR_2, ADS1115_ADDR_3};

// mux currently being converted on each chip
static uint8_t chip_mux_channel[] = {0, 0, 0};

static bool linux_adc_init()
{
  // we dont really care if we fail
  // since its not on baremetal linux handles it somewhat gracefully
  ads1115_init_chip(ADS1115_ADDR_1);
  ads1115_init_chip(ADS1115_ADDR_2);
  ads1115_init_chip(ADS1115_ADDR_3);
  return true;
}

static bool linux_start_conversion()
{
  for (int c= 0; c < adcGetMaxInputs(ADC_INPUT_MAIN); c++) {
//  for (int c = 0; c < ARRAY_SIZE(addrs); c++) {
    // Read the conversion result for the mux that was set in the previous cycle
    int16_t raw = ads1115_read_conv(addrs[c]);

    // Store to the correct channel
    // Chip 0 -> channels 0-3
    // Chip 1 -> channels 4-7
    // Chip 2 -> channels 8-11
    // Offset mux channel by 4 to skip pin-to-pin measurements and do pin-to-ground
    int channel = (c * AIN0_GND) + chip_mux_channel[c];
    setAnalogValue(channel, ads1115_scale(raw));
    //setAnalogValue(channel, rand()%ADC_MAX_VALUE);// for testing

    // Set next mux for the next cycle
    chip_mux_channel[c] = (chip_mux_channel[c] + 1) & 3;
    ads1115_set_mux(addrs[c], chip_mux_channel[c]);
  }

//  int max_input = adcGetMaxInputs(ADC_INPUT_ALL);
//  for (int i = ARRAY_SIZE(addrs)*4; i < max_input; i++) {
//    setAnalogValue(i, 2048);
//  }

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

// todo: read from /sys/class/rtc/rtc0/battery_volage and failsafe to 3v
uint16_t getRTCBatteryVoltage()
{
  return 300;
}

extern const etx_hal_adc_driver_t linux_adc_driver = {
  .inputs = _hal_inputs,
  .default_pots_cfg = _pot_default_config,
  .init = linux_adc_init,
  .start_conversion = linux_start_conversion,
  .wait_completion = nullptr,
};
