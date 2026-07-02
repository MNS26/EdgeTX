#pragma once

#include <cstdint>

int ads1115_open(const char* bus);

// Single-shot read of one multiplexer channel on an ADS1115 at the given I2C address.
// Returns signed 16-bit value (-32768 to 32767).
// Returns 0 on error (caller should check fd validity).
int16_t ads1115_read(int fd, uint8_t addr, uint8_t mux);

// Scale ADS1115 raw value (signed 16-bit) to EdgeTX 12-bit range (0-4095).
uint16_t ads1115_scale(int16_t raw);
