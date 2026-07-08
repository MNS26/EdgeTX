#include "touch_driver.h"

TouchState touchState = {};
static bool touchOccured = false;

bool touchPanelInit()
{
  touchState.x = touchState.y = 0;
  return true;
}

bool touchPanelEventOccured()
{
  if (touchOccured) {
    touchOccured = false;
    return true;
  }
  return false;
}

void touchPanelDown(short x, short y)
{
  touchState.x = x;
  touchState.y = y;
  touchState.event = TE_DOWN;
  touchOccured = true;
}

void touchPanelUp()
{
  touchState.event = TE_UP;
  touchOccured = true;
}

struct TouchState touchPanelRead()
{
  struct TouchState st = touchState;
  touchState.deltaX = 0;
  touchState.deltaY = 0;
  return st;
}

struct TouchState getInternalTouchState()
{
  return touchState;
}
