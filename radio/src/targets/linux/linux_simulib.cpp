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

#include "linux_simulib.h"
#include "linux.h"
#include "simulcd.h"
#include "serial_driver.h"

#include "hal/adc_driver.h"
#include "hal/rotary_encoder.h"
#include "hal/usb_driver.h"

#include "os/sleep.h"
#include "os/task.h"

#include "edgetx.h"
#include "debug.h"
#include "switches.h"
#include "input_mapping.h"
#include "gui/gui_common.h"
#include "mixes.h"
#if defined(GVARS)
#include "gvars.h"
#endif
#include "trainer.h"
#include "telemetry/frsky.h"
#include "telemetry/crossfire.h"
#if defined(LUA)
#include "lua/lua_api.h"
#endif

#include <assert.h>

int g_snapshot_idx = 0;

extern uint8_t startOptions;

char * main_thread_error = nullptr;

bool shutdown = false;
bool simu_shutdown = false; // TODO: remove dependent code
bool running = false;
bool CreateDefaultSettings = false;


volatile rotenc_t rotencValue = 0;
volatile uint32_t rotencDt = 0;

rotenc_t rotaryEncoderGetValue()
{
  return rotencValue / ROTARY_ENCODER_GRANULARITY;
}

const etx_hal_adc_driver_t* getDefaultAdcDriver();

void lcdCopy(void * dest, void * src);
void lcdFlushed();

void linuxInit()
{
#if defined(ROTARY_ENCODER_NAVIGATION)
  rotencValue = 0;
#endif

  traceCallback = simuTrace;

  adcInit(getDefaultAdcDriver());
  switchInit();

#if defined(AUX_SERIAL) || defined(AUX2_SERIAL)
  hostSerialInit();
#endif
}

bool keysStates[MAX_KEYS] = { false };
void simuSetKey(uint8_t key, bool state)
{
  assert(key < DIM(keysStates));
  keysStates[key] = state;
}

bool trimsStates[MAX_TRIMS * 2] = { false };
void simuSetTrim(uint8_t trim, bool state)
{
  assert(trim < DIM(trimsStates));
  trimsStates[trim] = state;
}

void simuCreateDefaults()
{
  CreateDefaultSettings = true;
}

void linuxStart(bool tests)
{
  if (running)
    return;

#if !defined(COLORLCD)
  menuLevel = 0;
#endif

  startOptions = (tests ? 0 : OPENTX_START_NO_SPLASH | OPENTX_START_NO_CALIBRATION | OPENTX_START_NO_CHECKS);
  shutdown = false;

  if (g_tmr10ms == 0) {
    g_tmr10ms = 1;
  }

#if defined(RTCLOCK)
  time_t rawtime;
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime (&rawtime);

  if (timeinfo != nullptr) {
    struct gtm gti;
    gti.tm_sec  = timeinfo->tm_sec;
    gti.tm_min  = timeinfo->tm_min;
    gti.tm_hour = timeinfo->tm_hour;
    gti.tm_mday = timeinfo->tm_mday;
    gti.tm_mon  = timeinfo->tm_mon;
    gti.tm_year = timeinfo->tm_year;
    gti.tm_wday = timeinfo->tm_wday;
    g_rtcTime = gmktime(&gti);
  } else {
    g_rtcTime = rawtime;
  }
#endif

  lcdInit();
  linuxMain();
  running = true;
}

extern task_handle_t mixerTaskId;
extern task_handle_t menusTaskId;
#if defined(AUDIO)
extern task_handle_t audioTaskId;
#endif

void linuxStop()
{
  if (!running)
    return;

  shutdown = true;
  task_shutdown_all();

  running = false;
}

bool linuxIsRunning()
{
  return running;
}

bool simuLcdChanged()
{
  bool changed = simuLcdRefresh;
  simuLcdRefresh = false;
  return changed;
}

uint32_t simuLcdCopy(uint8_t* buf, uint32_t maxLen)
{
  uint32_t size = DISPLAY_BUFFER_SIZE * sizeof(pixel_t);
  if (size > maxLen) size = maxLen;
  memcpy(buf, simuLcdBuf, size);
  return size;
}

uint32_t simuLcdGetWidth()
{
  return LCD_W;
}

uint32_t simuLcdGetHeight()
{
  return LCD_H;
}

uint32_t simuLcdGetDepth()
{
#if defined(COLORLCD)
  return 16;
#elif LCD_W == 212
  return 4;
#else
  return 1;
#endif
}

#if !defined(COLORLCD)
void lcdSetRefVolt(uint8_t val)
{
}
#endif

#if LCD_W == 128
void lcdSetInvert(bool invert)
{
}
#endif

uint32_t pwrCheck() { return shutdown ? e_power_off : e_power_on; }

bool pwrPressed() { return false; }
bool pwrOffPressed()
{
#if defined(PWR_BUTTON_PRESS)
  return pwrPressed();
#else
  return !pwrPressed();
#endif
}

void pwrInit() {}
void pwrOn() {}
void pwrOff() {}

bool UNEXPECTED_SHUTDOWN() { return false; }
void SET_POWER_REASON(uint32_t value) {}

#if defined(TRIMS_EMULATE_BUTTONS)
bool trimsAsButtons = false;

void setHatsAsKeys(bool val) { trimsAsButtons = val; }

bool getHatsAsKeys()
{
  bool lua = false;
#if defined(LUA)
  lua = isLuaStandaloneRunning();
#endif
  return (trimsAsButtons || lua);
}
#endif

// Called by the main EdgeTX code before reading keys.
// Under linux not used
void pollKeys()
{
}

int usbPlugged() { return false; }
int getSelectedUsbMode() { return USB_JOYSTICK_MODE; }
void setSelectedUsbMode(int mode) {}
void delay_ms(uint32_t ms) { }
void delay_us(uint16_t us) { }

void flashWrite(uint32_t *address, const uint32_t *buffer)
{
  sleep_ms(10);
}

uint32_t isBootloaderStart(const uint8_t * block)
{
  return 1;
}

#if defined(PCBXLITES)
bool isJackPlugged()
{
  return false;
}
#endif

void serialPrintf(const char * format, ...) { }
void serialCrlf() { }
void serialPutc(char c) { }

void boardOff()
{
}

void hapticOff() {}

#if !defined(HAPTIC_PWM)
void hapticOn() {}
#endif

#if defined(HAS_HARDWARE_OPTIONS)
HardwareOptions hardwareOptions;
#endif

uint32_t Master_frequency = 0;
uint32_t Current_used = 0;
uint16_t Current_max = 0;

void handleJackConnection() {}

int trainerModuleSbusGetByte(unsigned char*) { return 0; }

// stub: OS manages rtc
void rtcInit()
{
}

// hook into OS rtc to read time
// technically not needed
void rtcGetTime(struct gtm * t)
{
}

//stub: OS manages rtc
void rtcSetTime(const struct gtm * t)
{
}


uint32_t SD_GetCardType() { return 0; }

#if defined(USB_SERIAL)
const etx_serial_port_t UsbSerialPort = { "USB-VCP", nullptr, nullptr };
#endif


#if defined(HARDWARE_TOUCH)
void simuTouchDown(int16_t x, int16_t y)
{
  touchPanelDown(x, y);
}

void simuTouchUp()
{
  touchPanelUp();
}
#else
void simuTouchDown(int16_t x, int16_t y){}
void simuTouchUp(){}
#endif

void simuRotaryEncoderEvent(int32_t steps)
{
#if defined(ROTARY_ENCODER_NAVIGATION)
  rotencValue += steps * ROTARY_ENCODER_GRANULARITY;
#endif
}

int32_t simuGetCapability(uint8_t cap)
{
  switch (cap) {
    case 0:  // CAP_LUA
#ifdef LUA
      return 1;
#else
      return 0;
#endif
    case 1:  // CAP_ROTARY_ENC
      return 0;
    case 2:  // CAP_ROTARY_ENC_NAV
#ifdef ROTARY_ENCODER_NAVIGATION
      return 1;
#else
      return 0;
#endif
    case 3:  // CAP_TELEM_FRSKY_SPORT
      return 1;
    case 4:  // CAP_SERIAL_AUX1
      return (auxSerialGetPort(SP_AUX1) != nullptr) ? 1 : 0;
    case 5:  // CAP_SERIAL_AUX2
      return (auxSerialGetPort(SP_AUX2) != nullptr) ? 1 : 0;
    default:
      return 0;
  }
}

void simuSetTrimValue(uint8_t idx, int32_t value)
{
  unsigned i = inputMappingConvertMode(idx);
  uint8_t phase = getTrimFlightMode(getFlightMode(), i);
  setTrimValue(phase, i, value);
}

void simuSendTelemetry(uint8_t module, uint8_t protocol,
                       const uint8_t* data, uint32_t len)
{
  switch (protocol) {
    case 0:  // SIMU_TELEMETRY_PROTOCOL_FRSKY_SPORT
      sportProcessTelemetryPacket(module, data, len);
      break;
    case 1:  // SIMU_TELEMETRY_PROTOCOL_FRSKY_HUB
      frskyDProcessPacket(module, data, len);
      break;
    case 2:  // SIMU_TELEMETRY_PROTOCOL_CROSSFIRE
      processCrossfireTelemetryFrame(module, (uint8_t*)data, len);
      break;
    case 3:  // SIMU_TELEMETRY_PROTOCOL_FRSKY_HUB_OOB
      if (len >= 3) {
        uint8_t id = data[0];
        int16_t value = ((uint8_t)(data[2]) << 8) + (uint8_t)(data[1]);
        processHubPacket(id, value);
      }
      break;
    default:
      break;
  }
}

void simuLuaReloadPermanentScripts()
{
#if defined(LUA)
  luaState = INTERPRETER_RELOAD_PERMANENT_SCRIPTS;
#endif
}

void simuLcdFlushed()
{
  ::lcdFlushed();
}

uint8_t simuGetMaxTrainerChannels()
{
  return MAX_TRAINER_CHANNELS;
}

void simuCopyTrainerInput(const int16_t* buf, uint8_t count)
{
  if (count > MAX_TRAINER_CHANNELS)
    count = MAX_TRAINER_CHANNELS;
  for (uint8_t i = 0; i < count; i++) {
    int16_t v = buf[i];
    if (v < -512) v = -512;
    if (v > 512) v = 512;
    trainerInput[i] = v;
  }
}

void simuSetTrainerTimeout(uint16_t ms)
{
  trainerSetTimer(ms / 10);
}

// -- Output values --

uint8_t simuGetNumChannels()
{
  return MAX_OUTPUT_CHANNELS;
}

uint8_t simuCopyChannelOutputs(int16_t* buf, uint8_t maxCount)
{
  uint8_t n = MAX_OUTPUT_CHANNELS < maxCount ? MAX_OUTPUT_CHANNELS : maxCount;
  memcpy(buf, channelOutputs, n * sizeof(int16_t));
  return n;
}

uint8_t simuCopyMixOutputs(int16_t* buf, uint8_t maxCount)
{
  uint8_t n = MAX_OUTPUT_CHANNELS < maxCount ? MAX_OUTPUT_CHANNELS : maxCount;
  memcpy(buf, ex_chans, n * sizeof(int16_t));
  return n;
}

bool simuIsChannelUsed(uint8_t channel)
{
  return isChannelUsed(channel);
}

int simuGetChannelsUsed()
{
  return getChannelsUsed();
}

uint8_t simuGetMixCount()
{
  return getMixCount();
}

uint8_t simuGetNumLogicalSwitches()
{
  return MAX_LOGICAL_SWITCHES;
}

uint8_t simuCopyLogicalSwitches(uint8_t* buf, uint8_t maxCount)
{
  uint8_t n = MAX_LOGICAL_SWITCHES < maxCount ? MAX_LOGICAL_SWITCHES : maxCount;
  for (uint8_t i = 0; i < n; i++)
    buf[i] = getSwitch(SWSRC_FIRST_LOGICAL_SWITCH + i, 0) ? 1 : 0;
  return n;
}

int32_t simuGetTrimValue(uint8_t idx)
{
  uint8_t phase = getFlightMode();
  uint8_t mapped = inputMappingConvertMode(idx);
  return getTrimValue(getTrimFlightMode(phase, mapped), mapped);
}

int16_t simuGetTrimRange()
{
  return g_model.extendedTrims ? TRIM_EXTENDED_MAX : TRIM_MAX;
}

int32_t simuGetFlightMode()
{
  return getFlightMode();
}

uint8_t simuGetNumGVars()
{
#if defined(GVARS)
  return MAX_GVARS;
#else
  return 0;
#endif
}

uint8_t simuGetNumFlightModes()
{
  return MAX_FLIGHT_MODES;
}

int32_t simuGetGVar(uint8_t gv, uint8_t fm)
{
#if defined(GVARS)
  if (gv < MAX_GVARS && fm < MAX_FLIGHT_MODES) {
    uint8_t prec = g_model.gvars[gv].prec;
    uint8_t unit = g_model.gvars[gv].unit;
    int16_t value = (int16_t)GVAR_VALUE(gv, getGVarFlightMode(fm, gv));
    return (((unit & 0x3) << 26) | ((prec & 0x3) << 24) |
            ((fm & 0xFF) << 16) | (value & 0xFFFF));
  }
#endif
  return 0;
}

bool simuGetBacklightState()
{
  return isBacklightEnabled();
}

// -- Custom (function) switches --

uint8_t simuGetNumCustomSwitches()
{
  return NUM_FUNCTIONS_SWITCHES;
}

uint8_t simuGetCustomSwitchIndex(uint8_t cfsIdx)
{
#if defined(FUNCTION_SWITCHES)
  return switchGetSwitchFromCustomIdx(cfsIdx);
#else
  (void)cfsIdx;
  return 0;
#endif
}

bool simuGetCustomSwitchState(uint8_t idx)
{
#if defined(FUNCTION_SWITCHES)
  if (idx < NUM_FUNCTIONS_SWITCHES)
    return fsLedState(idx);
#endif
  return false;
}

uint32_t simuGetCustomSwitchColor(uint8_t idx)
{
#if defined(FUNCTION_SWITCHES)
  if (idx < NUM_FUNCTIONS_SWITCHES)
    return fsGetLedRGB(idx);
#endif
  return 0;
}

#if defined(PDM_CLOCK)
#include "boards/rm-h750/pdm_software_driver.h"
void pdmStart() {}
void pdmStop() {}
bool pdmUpdateSoundLevel() { return false; }
uint8_t pdmGetSoundLevel() { return 0; }
bool pdmCapture() { return false; }
uint32_t pdmConvertToPCM(int16_t*, uint32_t) { return 0; }
#endif

// USB stubs for linux target (no STM32 USB hardware)
void usbStart() {}
void usbStop() {}
bool usbStarted() { return false; }
void usbJoystickUpdate() {}
void usbJoystickRestart() {}

// DMA stub for linux target
void DMAInit() {}

// FatFs disk_ioctl stub
#include "hal/fatfs_diskio.h"
extern "C" DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
  if (cmd == GET_SECTOR_COUNT) {
    *(DWORD*)buff = 1024 * 1024; // fake 1M sectors
    return RES_OK;
  }
  return RES_OK;
}
