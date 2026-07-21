/*
 * Copyright (C) EdgeTX
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hal.h"
#include "dataconstants.h"
#include "hal/serial_driver.h"
#include "hal/serial_port.h"
#include "os/task.h"

#include "serial_driver.h"

#include <deque>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

struct host_serial_port_t {
  uint8_t index;
  mutex_handle_t rxMutex;
  std::deque<uint8_t> rxQueue;
  int fd = -1;
};

static host_serial_port_t hostSerialPorts[MAX_AUX_SERIAL] = {
  { SP_AUX1, {}, {} },
  { SP_AUX2, {}, {} },
};

static const char* getDevicePath(uint8_t port_nr)
{
  switch (port_nr) {
    case SP_AUX1: return AUX_SERIAL_PATH;
    case SP_AUX2: return AUX2_SERIAL_PATH;
    default: return nullptr;
  }
}

static bool portHasCTS(uint8_t port_nr)
{
#if defined(AUX1_SERIAL_CTS)
  if (port_nr == SP_AUX1) return true;
#endif
#if defined(AUX2_SERIAL_CTS)
  if (port_nr == SP_AUX2) return true;
#endif
  return false;
}

static speed_t baudToSpeed(uint32_t baud)
{
  switch (baud) {
    case 50: return B50;
    case 75: return B75;
    case 110: return B110;
    case 134: return B134;
    case 150: return B150;
    case 200: return B200;
    case 300: return B300;
    case 600: return B600;
    case 1200: return B1200;
    case 1800: return B1800;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 500000: return B500000;
    case 576000: return B576000;
    case 921600: return B921600;
    case 1000000: return B1000000;
    case 1152000: return B1152000;
    case 1500000: return B1500000;
    case 2000000: return B2000000;
    case 2500000: return B2500000;
    case 3000000: return B3000000;
    case 3500000: return B3500000;
    case 4000000: return B4000000;
    default: return B115200;
  }
}

static int openSerialPort(const char* path, uint32_t baudrate, bool hwflow)
{
  int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) return -1;

  struct termios tios;
  if (tcgetattr(fd, &tios) < 0) {
    close(fd);
    return -1;
  }

  cfmakeraw(&tios);

  speed_t speed = baudToSpeed(baudrate);
  cfsetispeed(&tios, speed);
  cfsetospeed(&tios, speed);

  tios.c_cflag &= ~CSIZE;
  tios.c_cflag |= CS8;
  tios.c_cflag &= ~(PARENB | CSTOPB);

  tios.c_iflag &= ~(IXON | IXOFF | IXANY);

  if (hwflow)
    tios.c_cflag |= CRTSCTS;
  else
    tios.c_cflag &= ~CRTSCTS;

  tios.c_cc[VMIN] = 0;
  tios.c_cc[VTIME] = 0;

  if (tcsetattr(fd, TCSANOW, &tios) < 0) {
    close(fd);
    return -1;
  }

  tcflush(fd, TCIOFLUSH);
  return fd;
}

#if !defined(__wasm__)

void simuAuxSerialStart(uint8_t port_nr, uint32_t baudrate, uint8_t)
{
  if (port_nr >= MAX_AUX_SERIAL) return;
  auto& port = hostSerialPorts[port_nr];

  const char* path = getDevicePath(port_nr);
  if (path == nullptr) return;

  port.fd = openSerialPort(path, baudrate, portHasCTS(port_nr));
}

void simuAuxSerialStop(uint8_t port_nr)
{
  if (port_nr >= MAX_AUX_SERIAL) return;
  auto& port = hostSerialPorts[port_nr];

  if (port.fd >= 0) {
    tcdrain(port.fd);
    close(port.fd);
    port.fd = -1;
  }
}

void simuAuxSerialSetBaudrate(uint8_t port_nr, uint32_t baudrate)
{
  if (port_nr >= MAX_AUX_SERIAL) return;
  auto& port = hostSerialPorts[port_nr];

  if (port.fd < 0) return;

  struct termios tios;
  if (tcgetattr(port.fd, &tios) < 0) return;

  speed_t speed = baudToSpeed(baudrate);
  cfsetispeed(&tios, speed);
  cfsetospeed(&tios, speed);
  tcsetattr(port.fd, TCSANOW, &tios);
}

void simuAuxSerialSendBuffer(uint8_t port_nr, const uint8_t* data, uint32_t len)
{
  if (port_nr >= MAX_AUX_SERIAL || data == nullptr || len == 0) return;
  auto& port = hostSerialPorts[port_nr];

  if (port.fd < 0) return;

  uint32_t sent = 0;
  while (sent < len) {
    ssize_t n = write(port.fd, data + sent, len - sent);
    if (n < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;
      break;
    }
    sent += n;
  }
}

#endif // !__wasm__

void hostSerialInit()
{
  for (uint8_t i = 0; i < MAX_AUX_SERIAL; ++i)
    mutex_create(&hostSerialPorts[i].rxMutex);
}

static void* host_drv_init(void* hw_def, const etx_serial_init* dev)
{
  if (hw_def == nullptr || dev == nullptr) return nullptr;

  auto* port = static_cast<host_serial_port_t*>(hw_def);
  mutex_lock(&port->rxMutex);
  port->rxQueue.clear();
  mutex_unlock(&port->rxMutex);
  simuAuxSerialStart(port->index, dev->baudrate, dev->encoding);
  return port;
}

static void host_drv_deinit(void* ctx)
{
  if (ctx == nullptr) return;
  auto* port = static_cast<host_serial_port_t*>(ctx);
  simuAuxSerialStop(port->index);
}

static void host_drv_send_byte(void* ctx, uint8_t b)
{
  if (ctx == nullptr) return;
  auto* port = static_cast<host_serial_port_t*>(ctx);
  simuAuxSerialSendBuffer(port->index, &b, 1);
}

static void host_drv_send_buffer(void* ctx, const uint8_t* b, uint32_t l)
{
  if (ctx == nullptr || b == nullptr || l == 0) return;
  auto* port = static_cast<host_serial_port_t*>(ctx);
  simuAuxSerialSendBuffer(port->index, b, l);
}

static bool host_drv_tx_completed(void*) { return true; }

static int host_drv_get_byte(void* ctx, uint8_t* b)
{
  if (ctx == nullptr || b == nullptr) return 0;
  auto* port = static_cast<host_serial_port_t*>(ctx);

  mutex_lock(&port->rxMutex);

  if (port->rxQueue.empty() && port->fd >= 0) {
    uint8_t buf[64];
    ssize_t n = read(port->fd, buf, sizeof(buf));
    if (n > 0) {
      for (ssize_t i = 0; i < n; ++i)
        port->rxQueue.push_back(buf[i]);
    }
  }

  if (port->rxQueue.empty()) {
    mutex_unlock(&port->rxMutex);
    return 0;
  }
  *b = port->rxQueue.front();
  port->rxQueue.pop_front();
  mutex_unlock(&port->rxMutex);
  return 1;
}

static void host_drv_set_baudrate(void* ctx, uint32_t baudrate)
{
  if (ctx == nullptr) return;
  auto* port = static_cast<host_serial_port_t*>(ctx);
  simuAuxSerialSetBaudrate(port->index, baudrate);
}

const etx_serial_driver_t host_drv = {
  .init = host_drv_init,
  .deinit = host_drv_deinit,
  .sendByte = host_drv_send_byte,
  .sendBuffer = host_drv_send_buffer,
  .txCompleted = host_drv_tx_completed,
  .waitForTxCompleted = nullptr,
  .enableRx = nullptr,
  .getByte = host_drv_get_byte,
  .getLastByte = nullptr,
  .getBufferedBytes = nullptr,
  .copyRxBuffer = nullptr,
  .clearRxBuffer = nullptr,
  .getBaudrate = nullptr,
  .setBaudrate = host_drv_set_baudrate,
  .setPolarity = nullptr,
  .setHWOption = nullptr,
  .setReceiveCb = nullptr,
  .setIdleCb = nullptr,
  .setBaudrateCb = nullptr,
};

void simuAuxSerialReceive(uint8_t port_nr, const uint8_t* data, uint32_t len)
{
  if (port_nr >= MAX_AUX_SERIAL || data == nullptr || len == 0) return;
  auto& port = hostSerialPorts[port_nr];
  mutex_lock(&port.rxMutex);
  for (uint32_t i = 0; i < len; ++i)
    port.rxQueue.push_back(data[i]);
  mutex_unlock(&port.rxMutex);
}

#if defined(AUX_SERIAL_PWR_GPIO)
static void null_pwr_aux(uint8_t) {}
#endif

#if defined(AUX_SERIAL)
#if defined(AUX_SERIAL_PWR_GPIO)
  #define AUX_SERIAL_PWR null_pwr_aux
#else
  #define AUX_SERIAL_PWR nullptr
#endif
static etx_serial_port_t auxSerialPort = {
  "AUX1",
  &host_drv,
  &hostSerialPorts[SP_AUX1],
  AUX_SERIAL_PWR
};
#define AUX_SERIAL_PORT &auxSerialPort
#else
#define AUX_SERIAL_PORT nullptr
#endif

#if defined(AUX2_SERIAL)
#if defined(AUX_SERIAL_PWR_GPIO)
  #define AUX2_SERIAL_PWR null_pwr_aux
#else
  #define AUX2_SERIAL_PWR nullptr
#endif
static etx_serial_port_t aux2SerialPort = {
  "AUX2",
  &host_drv,
  &hostSerialPorts[SP_AUX2],
  AUX2_SERIAL_PWR
};
#define AUX2_SERIAL_PORT &aux2SerialPort
#else
#define AUX2_SERIAL_PORT nullptr
#endif

etx_serial_port_t* serialPorts[MAX_AUX_SERIAL] = {
  AUX_SERIAL_PORT,
  AUX2_SERIAL_PORT,
};

const etx_serial_port_t* auxSerialGetPort(int port_nr)
{
  if (port_nr >= MAX_AUX_SERIAL) return nullptr;
  return serialPorts[port_nr];
}
