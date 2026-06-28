#include "board.h"
#include "battery_driver.h"
#include "touch_driver.h"
#include "hal/switch_driver.h"

void boardInit()
{
  switchInit();
  touchPanelInit();
  battery_charge_init();
}
