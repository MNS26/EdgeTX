#pragma once

#include "touch.h"

bool touchPanelInit();
void touchPanelDown(short x, short y);
void touchPanelUp();
bool touchPanelEventOccured();
struct TouchState touchPanelRead();
struct TouchState getInternalTouchState();
