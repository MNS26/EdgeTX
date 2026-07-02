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

#include "battery_driver.h"
#include "board.h"

#include <dirent.h>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>

static char battery_name[32] = {};
static char ac_name[32] = {};
static bool battery_found = false;
static bool ac_found = false;

static void scanPowerSupply()
{
  DIR* dir = opendir("/sys/class/power_supply");
  if (!dir)
    return;

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.') continue;

    std::string type_path =
        std::string("/sys/class/power_supply/") + entry->d_name + "/type";
    std::ifstream type_file(type_path);
    if (!type_file.is_open())
      continue;

    std::string type;
    type_file >> type;

    if (type == "Battery" && !battery_found) {
      strncpy(battery_name, entry->d_name, sizeof(battery_name) - 1);
      battery_found = true;
    } else if (type == "Mains" && !ac_found) {
      strncpy(ac_name, entry->d_name, sizeof(ac_name) - 1);
      ac_found = true;
    }
  }
  closedir(dir);
}

static int64_t readIntFromFile(const char* path)
{
  std::ifstream f(path);
  if (!f.is_open()) return -1;
  int64_t val;
  f >> val;
  return f.fail() ? -1 : val;
}

static std::string readStringFromFile(const char* path)
{
  std::ifstream f(path);
  if (!f.is_open()) return {};
  std::string val;
  f >> val;
  return val;
}

void battery_charge_init()
{
  scanPowerSupply();
}

uint16_t getBatteryVoltage()
{
  if (!battery_found)
    return BATTERY_MAX * 10;

  char path[128];
  snprintf(path, sizeof(path), "/sys/class/power_supply/%s/voltage_now",
           battery_name);

  int64_t uv = readIntFromFile(path);
  if (uv < 0)
    return BATTERY_MAX * 10;

  return uv / 10000;
}

bool isChargerActive()
{
  // Check if AC adapter is present and online
  if (ac_found) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/power_supply/%s/online",
             ac_name);
    int online = readIntFromFile(path);
    if (online == 1) return true;
  }

  // Fall back to checking battery charging status
  if (battery_found) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/power_supply/%s/status",
             battery_name);
    std::string status = readStringFromFile(path);
    return status == "Charging";
  }

  return false;
}

uint16_t get_battery_charge_state()
{
  if (isChargerActive()) return CHARGE_STARTED;
  return CHARGE_NONE;
}

bool usbChargerLed()
{
  return isChargerActive();
}

void handle_battery_charge(uint32_t) {}
void battery_charge_end() {}
