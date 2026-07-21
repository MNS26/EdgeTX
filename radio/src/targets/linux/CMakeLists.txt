if(NOT LINUX_BUILD)
  message(STATUS "Configuring Linux target")
  set(FLAVOUR "linux")
  set(TARGET_DIR "linux")
  set(GUI_DIR colorlcd)
  set(BITMAPS_DIR 800x480)
  set(BITMAPS_TARGET bm800_bitmaps)
  set(RADIO_DEPENDENCIES ${RADIO_DEPENDENCIES} ${BITMAPS_TARGET})
  set(CPU_TYPE STM32F4)
  set(PCBREV STD)
  set(TRAINER NO)
  set(ROTARY_ENCODER YES)
  set(USB_CHARGER YES)
  set(HARDWARE_TOUCH YES)
  set(STORAGE_MODELSLIST YES)
  set(HAPTIC ON)
  set(AUDIO ON)
  set(AUX_SERIAL ON)
  set(AUX2_SERIAL ON)
  option(MULTIMODULE "DIY Multiprotocol TX Module (https://github.com/pascallanger/DIY-Multiprotocol-TX-Module)" ON)
  set(DEFAULT_INTERNAL_MODULE MULTIMODULE CACHE STRING "Default internal module")

  # defines existing internal modules
  set(INTERNAL_MODULES PXX1;PXX2;MULTI;CRSF CACHE STRING "Internal modules")

  add_definitions(-DHARDWARE_KEYS)

  if(MULTIMODULE)
    add_definitions(-DMULTI_PROTOLIST)
  endif()
  add_definitions(-DHARDWARE_EXTERNAL_MODULE)
  add_definitions(-DHARDWARE_TOUCH)
  add_definitions(-DSOFTWARE_VOLUME)
  add_definitions(-DRTCLOCK)

  set(LINUX_BUILD TRUE)

  # Module/IO source files needed by colorlcd GUI
  set(SRC ${SRC}
    io/frsky_firmware_update.cpp
    io/multi_firmware_update.cpp
    io/multi_protolist.cpp
    io/bootloader_flash.cpp
  )

  message(STATUS "Linux target GUI_DIR=${GUI_DIR}")
  return()
endif()

# Build part — runs during add_subdirectory()
find_package(SDL2 QUIET COMPONENTS SDL2 CONFIG)
if(NOT SDL2_FOUND)
  find_package(SDL2 REQUIRED)
endif()

if(MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-ms-bitfields")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

if(ASAN AND NOT WASI)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")

# Driver files shared by both linux and standalone (simu) targets
set(LINUX_DRIVERS
  board.cpp
  touch_driver.cpp
  battery_driver.cpp
  linux_simulib.cpp
  linuxfatfs.cpp
  ../simu/simulcd.cpp
  audio_driver.cpp
  fs_watcher.cpp
  module_drivers.cpp
  serial_driver.cpp
  backlight_driver.cpp
  timers_driver.cpp
  linux_abnormal_reboot.cpp
)

# Real hardware sources (compiled only for the linux executable, without -DSIMU)
set(LINUX_HW_SRC
  adc_driver.cpp
  ads1115_driver.cpp
  mcp23017_driver.cpp
  linux_input.cpp
  switch_driver.cpp
)

# Simu include directories for shared headers
include_directories(../simu)

set(HW_DESC_JSON ${FLAVOUR}.json)
AddHWGenTarget(${HW_DESC_JSON} simu_keys simu_keys.inc)
AddHWGenTarget(${HW_DESC_JSON} simu_switches simu_switches.inc)
AddHWGenTarget(${HW_DESC_JSON} hal_adc_inputs hal_adc_inputs.inc)

set(LINUX_DRIVERS ${LINUX_DRIVERS}
  ${CMAKE_CURRENT_BINARY_DIR}/simu_keys.inc
  ${CMAKE_CURRENT_BINARY_DIR}/simu_switches.inc
  ${CMAKE_CURRENT_BINARY_DIR}/hal_adc_inputs.inc
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Pack all shared driver sources into an object lib (compiled with -DSIMU from radiolib_native)
add_library(linux_drivers OBJECT EXCLUDE_FROM_ALL
  ${LINUX_DRIVERS}
)

get_property(LINUX_SRC_OPTIONS
  TARGET radiolib_native
  PROPERTY INTERFACE_COMPILE_OPTIONS
)

target_compile_options(linux_drivers PRIVATE ${LINUX_SRC_OPTIONS})
set_property(TARGET linux_drivers PROPERTY POSITION_INDEPENDENT_CODE ON)
add_dependencies(linux_drivers bm800_bitmaps)

set(LINUX_SRC
  $<TARGET_OBJECTS:radiolib_native>
  $<TARGET_OBJECTS:linux_drivers>
)
set(LINUX_SRC ${LINUX_SRC} PARENT_SCOPE)

# Set the options as well in parent scope to be used by unit test
set(LINUX_SRC_OPTIONS ${LINUX_SRC_OPTIONS} PARENT_SCOPE)

# STB
set(STB_DIR ${RADIO_SRC_DIR}/thirdparty/stb)


add_executable(linux
  EXCLUDE_FROM_ALL
  ${LINUX_SRC}
  ${LINUX_HW_SRC}
  display.cpp
  audio.cpp

#  ../simu/simuaudio.cpp
#  ../simu/display.cpp
  ../simu/arg_parser.cpp
  sdl_linux.cpp
)

target_compile_options(linux PUBLIC -DRADIO_LINUX -DHARDWARE_REAL_INPUTS)

if(USB_CHARGER)
  target_compile_options(linux PUBLIC -DUSB_CHARGER)
  target_compile_options(radiolib_native PUBLIC -DUSB_CHARGER)
endif()

target_include_directories(linux PUBLIC ${STB_DIR})
target_link_libraries(linux PRIVATE SDL2::SDL2)

# Image assets (only once)
add_png_target(image_assets "assets/images/*.png")
add_dependencies(linux image_assets bm800_bitmaps yaml_data)

# STB
target_include_directories(linux PUBLIC ${STB_DIR})
