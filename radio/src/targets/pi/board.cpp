#include "board.h"
#include "touch_driver.h"
#include "hal/switch_driver.h"

void boardInit()
{
  switchInit();
  touchPanelInit();
}
