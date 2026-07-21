#pragma once

#include "definitions.h"
#include "edgetx_constants.h"

#define ROTARY_ENCODER_NAVIGATION

#define BOOTLOADER_KEYS 0x42

#include "board_common.h"
#include "hal.h"
#include "hal/serial_port.h"
#include "hal/watchdog_driver.h"




#if defined(HARDWARE_TOUCH)
struct TouchState touchPanelRead();
bool touchPanelEventOccured();
struct TouchState getInternalTouchState();
#endif

#define HAS_HARDWARE_OPTIONS

PACK(typedef struct {
  uint8_t pcbrev:2;
  uint8_t pxx2Enabled:1;
}) HardwareOptions;

extern HardwareOptions hardwareOptions;

#define FLASHSIZE                      0x200000
#define FLASH_PAGESIZE                 256
#define BOOTLOADER_SIZE                0x20000
#define FIRMWARE_ADDRESS               0x08000000

#define IS_FIRMWARE_COMPATIBLE_WITH_BOARD() true

// Trims driver
#define NUM_TRIMS                               6
#define NUM_TRIMS_KEYS                          (NUM_TRIMS * 2)

#define VOLUME_LEVEL_MAX 23
// Battery driver (6S LiPo defaults)
#define BATTERY_WARN      222
#define BATTERY_MIN       210
#define BATTERY_MAX       247
bool usbChargerLed();

// Backlight driver
#define BACKLIGHT_LEVEL_MAX     100
#define BACKLIGHT_FORCED_ON     BACKLIGHT_LEVEL_MAX + 1
#define BACKLIGHT_LEVEL_MIN     1
#define BACKLIGHT_ENABLE()     backlightEnable(BACKLIGHT_LEVEL_MAX)
#define BACKLIGHT_DISABLE()

extern bool boardBacklightOn;
void backlightInit();
void backlightEnable(uint8_t dutyCycle);
void backlightFullOn();
bool isBacklightEnabled();

// Audio driver
void audioInit();

// Haptic driver
void hapticInit();
void hapticDone();
void hapticOff();
#if defined(HAPTIC_PWM)
void hapticOn(uint32_t pwmPercent);
#else
void hapticOn();
#endif

// BT driver
#define BT_TX_FIFO_SIZE    64
#define BT_RX_FIFO_SIZE    256
#define BLUETOOTH_BOOTLOADER_BAUDRATE  230400
#define BLUETOOTH_FACTORY_BAUDRATE     57600
#define BLUETOOTH_DEFAULT_BAUDRATE     115200
void bluetoothInit(uint32_t baudrate, bool enable);
void bluetoothWriteWakeup();
uint8_t bluetoothIsWriting();
void bluetoothDisable();

// LED driver
void ledInit();
void ledOff();
void ledRed();
void ledBlue();

// Power
void pwrInit();
uint32_t pwrCheck();
void pwrOn();
void pwrOff();
void pwrResetHandler();
bool pwrPressed();
bool pwrOffPressed();

// Board driver
void boardInit();
void boardOff();

#define SLAVE_MODE()                   false

// LCD
void lcdInit();
void lcdSetInitalFrameBuffer(void* fbAddress);
#define lcdRefreshWait(...)

#define MB                             *1024*1024
#define LUA_MEM_EXTRA_MAX              (2 MB)
#define LUA_MEM_MAX                    (6 MB)

#define BATTERY_DIVIDER 1495

#define INTMODULE_FIFO_SIZE            512
#define TELEMETRY_FIFO_SIZE            512
void telemetryPortInit(uint32_t baudrate, uint8_t mode);
void telemetryPortSetDirectionInput();
void telemetryPortSetDirectionOutput();
void sportSendByte(uint8_t byte);
void sportSendBuffer(const uint8_t * buffer, uint32_t count);
bool sportGetByte(uint8_t * byte);
void telemetryClearFifo();
extern uint32_t telemetryErrors;

#define DEBUG_BAUDRATE                  460800
#define LUA_DEFAULT_BAUDRATE            115200

const etx_serial_port_t* auxSerialGetPort(int port_nr);

#define INTERNAL_MODULE_ON()
#define INTERNAL_MODULE_OFF()
#define EXTERNAL_MODULE_ON()
#define EXTERNAL_MODULE_OFF()

#define NUM_FUNCTIONS_SWITCHES 0
