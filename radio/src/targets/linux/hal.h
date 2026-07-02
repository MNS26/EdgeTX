#pragma once

#include "definitions.h"

#define LCD_W                          800
#define LCD_H                          480
#define LCD_PHYS_H                     LCD_H
#define LCD_PHYS_W                     LCD_W
#define LCD_DEPTH                      16

#if defined(ROTARY_ENCODER)
#include "hal/rotary_encoder.h"
#endif
