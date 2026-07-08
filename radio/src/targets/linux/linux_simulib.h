/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include <stdint.h>
#include <string>

#include "hal/rotary_encoder.h"

// Lifecycle: call linuxInit() once, then fatfsSetPath() + linuxStart().
void linuxInit();
void linuxStart(bool tests = true);
void linuxStop();
bool linuxIsRunning();

// Internal: called by linuxStart(), defined in edgetx.cpp
// void main in firmware, renamed when not under firmware
void linuxMain();

// Set SD card and settings paths before linuxStart()
void fatfsSetPath(const char* sdPath, const char* settingsPath);
std::string fatfsGetCurrentPath();
std::string fatfsGetRealPath(const std::string& p);

// Input: keys use Board::Keys enum, switches use Board switch indices,
// trims use Board::TrimSwitches enum (momentary press, not value).
void simuSetKey(uint8_t key, bool state);
void simuSetTrim(uint8_t trim, bool state);
void simuSetSwitch(uint8_t swtch, int8_t state);


#if defined(HARDWARE_TOUCH)
  extern struct TouchState simTouchState;
  extern bool simTouchOccured;
#endif

// State flags
extern bool shutdown;
extern bool running;
extern bool simu_shutdown;

// Audio: queue audio data for host playback
void simuQueueAudio(const uint8_t* buf, uint32_t len);

// Trace: send debug output to host
void simuTrace(const char* text);

// Rotary encoder
extern volatile rotenc_t rotencValue;
extern volatile uint32_t rotencDt;
