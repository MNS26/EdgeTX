#include "ads1115_driver.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int ads1115_open(const char* bus)
{
  return open(bus, O_RDWR);
}

int16_t ads1115_read(int fd, uint8_t addr, uint8_t mux)
{
  if (fd < 0) return 0;
  if (ioctl(fd, I2C_SLAVE, addr) < 0) return 0;

  // Config: start single-shot, ±4.096V, 860SPS, comparator disabled
  uint16_t config = (1 << 15)          // OS = start conversion
                  | ((mux & 3) << 12)  // MUX
                  | (1 << 9)           // PGA = ±4.096V
                  | (1 << 8)           // MODE = single-shot
                  | (7 << 5)           // DR = 860 SPS
                  | (3 << 0);          // COMP_QUE = disable

  uint8_t buf[3] = {0x01, static_cast<uint8_t>((config >> 8) & 0xFF),
                    static_cast<uint8_t>(config & 0xFF)};
  if (write(fd, buf, 3) != 3) return 0;

  // Poll OS bit until conversion completes
  int timeout = 50;
  while (timeout--) {
    usleep(100);
    buf[0] = 0x01;
    if (write(fd, buf, 1) != 1) return 0;
    if (read(fd, buf, 2) != 2) return 0;
    if (buf[0] & 0x80) break;
  }

  // Read conversion register
  buf[0] = 0x00;
  if (write(fd, buf, 1) != 1) return 0;
  if (read(fd, buf, 2) != 2) return 0;

  return (buf[0] << 8) | buf[1];
}

uint16_t ads1115_scale(int16_t raw)
{
  if (raw < 0) raw = 0;
  return (uint16_t)((uint32_t)raw * 4095 / 32767);
}
