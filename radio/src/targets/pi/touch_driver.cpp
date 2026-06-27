#include "touch_driver.h"

static TouchState piTouchState = {};
static bool piTouchOccured = false;

bool touchPanelInit()
{
  piTouchState.x = piTouchState.y = 0;
  return true;
}

bool touchPanelEventOccured()
{
  if (piTouchOccured) {
    piTouchOccured = false;
    return true;
  }
  return false;
}

void touchPanelDown(short x, short y)
{
  piTouchState.x = x;
  piTouchState.y = y;
  piTouchState.event = TE_DOWN;
  piTouchOccured = true;
}

void touchPanelUp()
{
  piTouchState.event = TE_UP;
  piTouchOccured = true;
}

struct TouchState touchPanelRead()
{
  struct TouchState st = piTouchState;
  piTouchState.deltaX = 0;
  piTouchState.deltaY = 0;
  return st;
}

struct TouchState getInternalTouchState()
{
  return piTouchState;
}
