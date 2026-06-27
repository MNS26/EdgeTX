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
  set(HARDWARE_TOUCH YES)
  set(STORAGE_MODELSLIST YES)
  set(HAPTIC ON)
  set(AUDIO ON)
  option(MULTIMODULE "DIY Multiprotocol TX Module" ON)

  add_definitions(-DHARDWARE_KEYS)

  if(MULTIMODULE)
    add_definitions(-DMULTI_PROTOLIST)
  endif()
  add_definitions(-DHARDWARE_EXTERNAL_MODULE)
  add_definitions(-DHARDWARE_TOUCH)
  add_definitions(-DSOFTWARE_VOLUME)

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

option(SIMU_TARGET "Configure pi target" ON)
if(NOT SIMU_TARGET)
  message(STATUS "pi target disabled")
  return()
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

add_executable(simu
  EXCLUDE_FROM_ALL
  ${PI_SRC}
  ../simu/simuaudio.cpp
  ../simu/display.cpp
  ../simu/widgets.cpp
  ../simu/knobs.cpp
  ../simu/arg_parser.cpp
  ../simu/sdl_simu.cpp
)

target_compile_options(simu PUBLIC -DSIMU)

add_png_target(image_assets "../simu/assets/images/*.png")
add_dependencies(simu image_assets bm800_bitmaps yaml_data)

# STB
set(STB_DIR ${RADIO_SRC_DIR}/thirdparty/stb)
target_include_directories(simu PUBLIC ${STB_DIR})

# ImGui
include(FetchImgui)
target_link_libraries(simu PRIVATE imgui)
