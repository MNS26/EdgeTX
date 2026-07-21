#include "hal.h"
#include "ads1115_driver.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <exception>
#include <iostream>

#define I2C_BUS "/dev/i2c-1"

static int ads1115_bus_fd = -1;

int ads1115_open(const char* bus)
{
  return open(bus, O_RDWR);
}

static int ads1115_open_bus()
{
  if (ads1115_bus_fd >= 0) return ads1115_bus_fd;
  ads1115_bus_fd = ads1115_open(I2C_BUS);
  return ads1115_bus_fd;
}

static bool ads1115_select_slave(uint8_t addr)
{
  return ioctl(ads1115_bus_fd, I2C_SLAVE, addr) >= 0;
}

static bool ads1115_write_reg(uint8_t reg, uint16_t val)
{
  uint8_t buf[3] = { reg,
                     static_cast<uint8_t>((val >> 8) & 0xFF),
                     static_cast<uint8_t>(val & 0xFF) };
  return write(ads1115_bus_fd, buf, 3) == 3;
}

static bool ads1115_read_reg(uint8_t reg, uint16_t& val)
{
  if (write(ads1115_bus_fd, &reg, 1) != 1) return false;
  uint8_t buf[2] = {};
  if (read(ads1115_bus_fd, buf, 2) != 2) return false;
  val = (buf[0] << 8) | buf[1];
  return true;
}

static uint16_t ads1115_build_config(uint8_t mux)
{
  // Continuous conversion, 860 SPS, FSR +/-6.144V
  // OS=0 (no effect in continuous mode), MODE=0 (continuous)
  // COMP_QUE=11 (comparator disabled)
  return (0 << 15) |
         ((mux & 7) << 12) |
         (ads1115_programmable_gain_aplifier::fsr_6144 << 9) |
         (ads1115_operating_mode::continuos << 8) |
         (ads1115_data_rate::sps_860 << 5) |
         (ads1115_comparator_queue::disable_comoparator_queue << 0);
}

int ads1115_init_chip(uint8_t addr)
{
  if (ads1115_open_bus() < 0) return -1;
  if (!ads1115_select_slave(addr)) return -1;

  uint16_t config = ads1115_build_config(0);
  if (!ads1115_write_reg(address_config, config))
    return -1;

  return 0;
}

int16_t ads1115_read_conv(uint8_t addr)
{
  if (ads1115_bus_fd < 0) return 0;
  if (!ads1115_select_slave(addr)) return 0;
  uint16_t raw = 0;
  if (!ads1115_read_reg(address_conversion, raw))
    return 0;
  return (int16_t)raw;
}

bool ads1115_set_mux(uint8_t addr, uint8_t mux)
{
  if (ads1115_bus_fd < 0) return false;
  if (!ads1115_select_slave(addr)) return false;
  uint16_t config = ads1115_build_config(mux);
  return ads1115_write_reg(address_config, config);
}

uint16_t ads1115_scale(int16_t raw)
{
  if (raw < 0) raw = 0;
  return (uint16_t)((uint32_t)raw * 4095 / 32767);
}
