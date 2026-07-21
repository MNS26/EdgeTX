#pragma once

#include <cstdint>

#define ADS1115_ADDR_1 0x48
#define ADS1115_ADDR_2 0x49
#define ADS1115_ADDR_3 0x4A

enum ads1115_address_pointer {
  address_conversion,
  address_config,
  address_lo_thresh,
  address_hi_thresh
};


enum ads1115_mux {
  AIN0_AIN1,
  AIN0_AIN3,
  AIN1_AIN3,
  AIN2_AIN3,
  AIN0_GND,
  AIN1_GND,
  AIN2_GND,
  AIN3_GND
};

enum ads1115_programmable_gain_aplifier{
  fsr_6144,
  fsr_4096,
  fsr_2048, // default
  fsr_1024,
  fsr_0512,
  fsr_0256,


};

enum ads1115_operating_mode{
  continuos,
  single_shot, // default
};

enum ads1115_data_rate{
  sps_8,
  sps_16,
  sps_32,
  sps_64,
  sps_128, // default
  sps_250,
  sps_475,
  sps_860
};

enum ads1115_comparator_mode {
  traditional,
  window
};

enum ads1115_comparator_polarity {
  active_low,
  active_high
};

enum ads1115_latching_comparator {
  non_latching, // default
  latching
};

enum ads1115_comparator_queue {
  assert_after_1,
  assert_after_2,
  assert_after_4,
  disable_comoparator_queue // default
};

// Open the I2C bus device
int ads1115_open(const char* bus);

// Initialize a single ADS1115 chip at the given I2C address for continuous conversion.
// The chip starts converting on mux 0 immediately.
// Returns 0 on success, -1 on error.
int ads1115_init_chip(uint8_t addr);

// Read the conversion register (returns the latest completed conversion result).
// Selects the given I2C slave internally.
int16_t ads1115_read_conv(uint8_t addr);

// Switch the chip to a new mux in continuous conversion mode.
// The chip starts converting on this mux; the result is ready after ~1.16ms (at 860 SPS).
// Selects the given I2C slave internally.
bool ads1115_set_mux(uint8_t addr, uint8_t mux);

// Scale ADS1115 raw value (signed 16-bit) to EdgeTX 12-bit range (0-4095).
uint16_t ads1115_scale(int16_t raw);
