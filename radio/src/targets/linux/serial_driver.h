/*
 * Copyright (C) EdgeTX
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 */

#pragma once

#include <stdint.h>
#include "hal/serial_driver.h"
#include "hal/serial_port.h"

// Initialize host serial port mutexes. Call once from linuxInit().
void hostSerialInit();

// Linux termios-based serial driver for AUX serial ports.
extern const etx_serial_driver_t host_drv;

// Feed data into an AUX port's RX queue (called by external code).
void simuAuxSerialReceive(uint8_t port_nr, const uint8_t* data, uint32_t len);

// Get an AUX serial port by index (SP_AUX1 / SP_AUX2).
const etx_serial_port_t* auxSerialGetPort(int port_nr);
