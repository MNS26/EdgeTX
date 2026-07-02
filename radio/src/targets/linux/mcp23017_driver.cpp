#include "mcp23017_driver.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// MCP23017 register addresses
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPPUA  0x0C
#define MCP23017_GPPUB  0x0D
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13
#define MCP23017_OLATA  0x14
#define MCP23017_OLATB  0x15

static int write_reg(int fd, uint8_t addr, uint8_t reg, uint8_t val)
{
  if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
  uint8_t buf[2] = {reg, val};
  if (write(fd, buf, 2) != 2) return -1;
  return 0;
}

static int read_reg16(int fd, uint8_t addr, uint8_t reg_low, uint16_t* val)
{
  if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
  uint8_t reg = reg_low;
  if (write(fd, &reg, 1) != 1) return -1;
  uint8_t buf[2];
  if (read(fd, buf, 2) != 2) return -1;
  // Sequential read: reg_low first, then reg_low+1
  *val = (buf[1] << 8) | buf[0];
  return 0;
}

int mcp23017_open(const char* bus)
{
  return open(bus, O_RDWR);
}

void mcp23017_set_dir(int fd, uint8_t addr, uint16_t iodir)
{
  write_reg(fd, addr, MCP23017_IODIRA, iodir & 0xFF);
  write_reg(fd, addr, MCP23017_IODIRB, (iodir >> 8) & 0xFF);
}

uint16_t mcp23017_read_gpio(int fd, uint8_t addr)
{
  uint16_t val = 0;
  read_reg16(fd, addr, MCP23017_GPIOA, &val);
  return val;
}

void mcp23017_write_gpio(int fd, uint8_t addr, uint16_t gpio)
{
  write_reg(fd, addr, MCP23017_OLATA, gpio & 0xFF);
  write_reg(fd, addr, MCP23017_OLATB, (gpio >> 8) & 0xFF);
}

void mcp23017_set_pullups(int fd, uint8_t addr, uint16_t gppu)
{
  write_reg(fd, addr, MCP23017_GPPUA, gppu & 0xFF);
  write_reg(fd, addr, MCP23017_GPPUB, (gppu >> 8) & 0xFF);
}
