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



// UART device paths (configured via udev rules for consisten paths)
#define INTERNAL_SERIAL_PATH  "/dev/ttyINTERNAL"
#define EXTERNAL_SERIAL_PATH  "/dev/ttyEXTERNAL"

#if defined(AUX_SERIAL)
#define AUX_SERIAL_PATH "/dev/ttyAUX1"
#if defined(AUX2_SERIAL)
#define AUX2_SERIAL_PATH "/dev/ttyAUX2"
#endif
#endif

// Hardware flow control - only one AUX UART has CTS exposed
#define AUX1_SERIAL_CTS

#if defined(ROTARY_ENCODER)
#include "hal/rotary_encoder.h"
#endif
