#pragma once

#include <cstdint>

int mcp23017_open(const char* bus);

// Set IODIR register: bit = 1 means input, bit = 0 means output
void mcp23017_set_dir(int fd, uint8_t addr, uint16_t iodir);

// Read both GPIOA and GPIOB registers as a single 16-bit value
// Lower byte = GPIOA, upper byte = GPIOB
uint16_t mcp23017_read_gpio(int fd, uint8_t addr);

// Write both GPIOA and GPIOB output registers
void mcp23017_write_gpio(int fd, uint8_t addr, uint16_t gpio);

// Enable internal pull-up resistors on selected pins
void mcp23017_set_pullups(int fd, uint8_t addr, uint16_t gppu);
