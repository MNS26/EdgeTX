# Configuration part — runs during include() from parent scope
if(NOT PI_BUILD)
  message(STATUS "Configuring Pi target")
  set(FLAVOUR pi)
  set(TARGET_DIR pi)
  set(GUI_DIR colorlcd)
  set(BITMAPS_DIR 800x480)
  set(CPU_TYPE STM32F4)
  set(CPU_TYPE_FULL STM32F429xI)
  set(PCBREV STD)
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

  set(PI_BUILD TRUE)

  # Module/IO source files needed by colorlcd GUI
  set(SRC ${SRC}
    io/frsky_firmware_update.cpp
    io/multi_firmware_update.cpp
    io/multi_protolist.cpp
    io/bootloader_flash.cpp
  )

  message(STATUS "Pi target GUI_DIR=${GUI_DIR}")
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

# Pi drivers: pi-specific sources + simu stubs
set(PI_DRIVERS
  board.cpp
  touch_driver.cpp
  battery_driver.cpp
  pi_simulib.cpp
  ../simu/simufatfs.cpp
  ../simu/simudisk.cpp
  ../simu/simulcd.cpp
  ../simu/audio_driver.cpp
  ../simu/switch_driver.cpp
  ../simu/adc_driver.cpp
  ../simu/module_drivers.cpp
  ../simu/led_driver.cpp
  backlight_driver.cpp
  ../simu/bt_driver.cpp
  ../simu/timers_driver.cpp
  ../simu/abnormal_reboot.cpp
)

# Simu include directories for shared headers
include_directories(../simu)

set(HW_DESC_JSON ${FLAVOUR}.json)
AddHWGenTarget(${HW_DESC_JSON} simu_keys simu_keys.inc)
AddHWGenTarget(${HW_DESC_JSON} simu_switches simu_switches.inc)
AddHWGenTarget(${HW_DESC_JSON} hal_adc_inputs hal_adc_inputs.inc)

set(PI_DRIVERS ${PI_DRIVERS}
  ${CMAKE_CURRENT_BINARY_DIR}/simu_keys.inc
  ${CMAKE_CURRENT_BINARY_DIR}/simu_switches.inc
  ${CMAKE_CURRENT_BINARY_DIR}/hal_adc_inputs.inc
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Pack all pi driver sources into an object lib
add_library(pi_drivers OBJECT EXCLUDE_FROM_ALL
  ${PI_DRIVERS}
)

get_property(PI_SRC_OPTIONS
  TARGET radiolib_native
  PROPERTY INTERFACE_COMPILE_OPTIONS
)

target_compile_options(pi_drivers PRIVATE ${PI_SRC_OPTIONS})
set_property(TARGET pi_drivers PROPERTY POSITION_INDEPENDENT_CODE ON)
add_dependencies(pi_drivers bm800_bitmaps)

# Combine radiolib_native + pi_drivers
set(PI_SRC
  $<TARGET_OBJECTS:radiolib_native>
  $<TARGET_OBJECTS:pi_drivers>
)
set(PI_SRC ${PI_SRC} PARENT_SCOPE)

# Image assets (only once)
#if(NOT TARGET image_assets)
#  add_png_target(image_assets "../simu/assets/images/*.png")
#endif()
add_png_target(image_assets "assets/images/*.png")

# STB
set(STB_DIR ${RADIO_SRC_DIR}/thirdparty/stb)

# Pi standalone executable (no ImGui, direct SDL)
add_executable(pi
  EXCLUDE_FROM_ALL
  ${PI_SRC}
  ../simu/simuaudio.cpp
  ../simu/display.cpp
  ../simu/arg_parser.cpp
  sdl_pi.cpp
)

target_compile_options(pi PUBLIC -DSIMU)
if(USB_CHARGER)
  target_compile_options(pi PUBLIC -DUSB_CHARGER)
  target_compile_options(radiolib_native PUBLIC -DUSB_CHARGER)
endif()
target_include_directories(pi PUBLIC ${STB_DIR})
target_link_libraries(pi PRIVATE SDL2::SDL2)
add_dependencies(pi image_assets bm800_bitmaps yaml_data)

# STB
target_include_directories(pi PUBLIC ${STB_DIR})

# Pi simu executable (ImGui chrome, for debugging)
option(PI_SIMU "Build pi simu with ImGui chrome" OFF)
if(PI_SIMU)
  add_executable(standalone
    EXCLUDE_FROM_ALL
    ${PI_SRC}
    ../simu/simuaudio.cpp
    ../simu/display.cpp
    ../simu/widgets.cpp
    ../simu/knobs.cpp
    ../simu/arg_parser.cpp
    ../simu/sdl_simu.cpp
  )

  target_compile_options(standalone PUBLIC -DSIMU)
  if(USB_CHARGER)
    target_compile_options(standalone PUBLIC -DUSB_CHARGER)
  endif()
  target_include_directories(standalone PUBLIC ${STB_DIR})
  target_link_libraries(standalone PRIVATE SDL2::SDL2)
  add_dependencies(standalone image_assets bm800_bitmaps yaml_data)

  # STB
  target_include_directories(standalone PUBLIC ${STB_DIR})

  include(FetchImgui)
  target_link_libraries(standalone PRIVATE imgui)
endif()
