#pragma once

#include <cstdint>

#define MCP23017_ADDR_KEYS      0x20
#define MCP23017_ADDR_TRIMS     0x21
#define MCP23017_ADDR_SWITCHES  0x22
#define MCP23017_I2C_BUS        "/dev/i2c-1"

// Initialize and configure the MCP23017 chips
int mcp23017_init_bus();

int mcp23017_open(const char* bus);

// Get the bus file descriptor (for checking init state)
int mcp23017_get_bus_fd();

// Set IODIR register: bit = 1 means input, bit = 0 means output
void mcp23017_set_dir(int fd, uint8_t addr, uint16_t iodir);

// Read both GPIOA and GPIOB registers as a single 16-bit value
// Lower byte = GPIOA, upper byte = GPIOB
uint16_t mcp23017_read_gpio(int fd, uint8_t addr);

// Write both GPIOA and GPIOB output registers
void mcp23017_write_gpio(int fd, uint8_t addr, uint16_t gpio);

// Enable internal pull-up resistors on selected pins
void mcp23017_set_pullups(int fd, uint8_t addr, uint16_t gppu);
