/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_keycode.h>

#include "stb_image.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <filesystem>

#include "arg_parser.h"
#include "display.h"
#include "hal/adc_driver.h"
#include "hal/key_driver.h"
#include "simu.h"
#include "simuaudio.h"
#include "simulib.h"
#include "edgetx.h"

#if defined(ROTARY_ENCODER_NAVIGATION)
#include "hal/rotary_encoder.h"
extern volatile rotenc_t rotencValue;
#endif

#define TIMER_INTERVAL 10 // 10ms

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* screen_texture;

#if !defined(__EMSCRIPTEN__)
static const unsigned char _icon_png[] = {
#include "icon.lbm"
};
#endif

static bool app_running = false;

static bool handleKeyEvent(const SDL_Event& event)
{
  if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
  return false;

  const auto& key_event = event.key;
  bool key_handled = false;
  uint8_t key = 0;



  switch (key_event.keysym.sym) {

    case SDLK_ESCAPE:
      key = KEY_EXIT;
      key_handled = true;
      break;

    case SDLK_RETURN:
      key = KEY_ENTER;
      key_handled = true;
      break;

    case SDLK_LEFT:
      if (keysGetSupported() & (1 << KEY_LEFT)) {
        key = KEY_LEFT;
        key_handled = true;
      }
      break;

    case SDLK_RIGHT:
      if (keysGetSupported() & (1 << KEY_RIGHT)) {
        key = KEY_RIGHT;
        key_handled = true;
      }
      break;

    case SDLK_UP:
#if defined(ROTARY_ENCODER_NAVIGATION)
      if (event.type == SDL_KEYDOWN)
        rotencValue -= ROTARY_ENCODER_GRANULARITY;
#else
      if (keysGetSupported() & (1 << KEY_UP)) {
        key = KEY_UP;
        key_handled = true;
      }
#endif
      break;

    case SDLK_DOWN:
#if defined(ROTARY_ENCODER_NAVIGATION)
      if (event.type == SDL_KEYDOWN)
        rotencValue += ROTARY_ENCODER_GRANULARITY;
#else
      if (keysGetSupported() & (1 << KEY_DOWN)) {
        key = KEY_DOWN;
        key_handled = true;
      }
#endif
      break;

    case SDLK_PLUS:
      if (keysGetSupported() & (1 << KEY_PLUS)) {
        key = KEY_PLUS;
        key_handled = true;
      }
      break;

    case SDLK_MINUS:
      if (keysGetSupported() & (1 << KEY_MINUS)) {
        key = KEY_MINUS;
        key_handled = true;
      }
      break;

    case SDLK_PAGEUP:
      if (keysGetSupported() & (1 << KEY_PAGEUP)) {
        key = KEY_PAGEUP;
        key_handled = true;
      }
      break;

    case SDLK_PAGEDOWN:
      if (keysGetSupported() & (1 << KEY_PAGEDN)) {
        key = KEY_PAGEDN;
        key_handled = true;
      }
      break;

    case SDLK_m:
      if (keysGetSupported() & (1 << KEY_MENU)) {
        key = KEY_MENU;
        key_handled = true;
      } else if (keysGetSupported() & (1 << KEY_MODEL)) {
        key = KEY_MODEL;
        key_handled = true;
      }
      break;

    case SDLK_s:
      if (keysGetSupported() & (1 << KEY_SYS)) {
        key = KEY_SYS;
        key_handled = true;
      }
      break;

    case SDLK_t:
      if (keysGetSupported() & (1 << KEY_TELE)) {
        key = KEY_TELE;
        key_handled = true;
      }
      break;

    case SDLK_r:
      if (event.type == SDL_KEYUP) {
        simuStop();
        simuStart();
      }
      break;

    default:
      break;
  }

  if (key_handled)
    simuSetKey(key, key_event.type == SDL_KEYDOWN);

  return key_handled;
}

static void redraw();

static bool handleEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (handleKeyEvent(event)) continue;
#if defined(HARDWARE_TOUCH)
    if (event.type == SDL_MOUSEBUTTONDOWN ||
        event.type == SDL_MOUSEMOTION) {
      if (event.button.button == SDL_BUTTON_LEFT ||
          (event.motion.state & SDL_BUTTON_LMASK)) {
        int x, y, ww, wh;
        SDL_GetMouseState(&x, &y);
        SDL_GetWindowSize(window, &ww, &wh);
        touchPanelDown(x * LCD_W / ww, y * LCD_H / wh);
      }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
      if (event.button.button == SDL_BUTTON_LEFT)
        touchPanelUp();
    }
#endif
    if (event.type == SDL_QUIT) {app_running = false; return false;}

    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(window))
      return false;
  }
  redraw();
  return true;
}

static SDL_Surface* LoadImage(const unsigned char* pixels, size_t len)
{
  // Read data
  int32_t w, h, bpp;

  void* data = stbi_load_from_memory(pixels, len, &w, &h, &bpp, 0);
  if (!data) return NULL;

  Uint32 format = (bpp == 3) ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA32;

  SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, bpp * 8, format);
  if (surface) {
    SDL_LockSurface(surface);
    memcpy(surface->pixels, data, w * h * bpp);
    SDL_UnlockSurface(surface);
  }

  stbi_image_free(data);
  return surface;
}

static void redraw()
{
  refreshDisplay(screen_texture);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, screen_texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[])
{
  auto progname = std::filesystem::path(argv[0]).filename();
  ArgumentParser args(progname.string());
  if (!args.parse(argc, argv)) return 1;

  if (args.isHelpRequested()) {
    args.printHelp();
    return 0;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
    SDL_Log("SDL_Init: %s", SDL_GetError());
    return 1;
  }

  if (!simuAudioInit()) {
    SDL_Log("simuAudioInit failed — continuing without audio");
  }

  auto window_flags = SDL_WINDOW_BORDERLESS;
  window = SDL_CreateWindow("EdgeTX Pi",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             LCD_W, LCD_H,
                             window_flags);
  if (!window) {
    SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     LCD_W, LCD_H);
  if (!screen_texture) {
    SDL_Log("SDL_CreateTexture: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

#if !defined(__EMSCRIPTEN__)
  SDL_Surface* sdl_icon = LoadImage(_icon_png, sizeof(_icon_png));
  if (window && sdl_icon) {
    SDL_SetWindowIcon(window, sdl_icon);
    SDL_FreeSurface(sdl_icon);
  }
#endif

  simuInit();
  simuFatfsSetPaths(args.getStoragePath().c_str(),
                    args.getSettingsPath().c_str());
  simuStart();

  // Main Loop
  SDL_SetEventFilter([](void*, SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT &&
    (event->window.event == SDL_WINDOWEVENT_EXPOSED)) {
      redraw();
      return 0;
    }
    return 1;
  }, NULL);

  app_running = true;
  while (app_running) {
    Uint64 start_ts = SDL_GetPerformanceCounter();
    if (!handleEvents()) break;

//    SDL_Event event;
//    while (SDL_PollEvent(&event)) {
//      if (handleKeyEvent(event))
//        continue;
//
//      if (event.type == SDL_QUIT) {
//        app_running = false;
//        break;
//      }
//
//      if (event.type == SDL_WINDOWEVENT &&
//          event.window.event == SDL_WINDOWEVENT_CLOSE) {
//        app_running = false;
//        break;
//      }


//    }

    if (!app_running) break;

//    redraw();

    Uint64 end_ts = SDL_GetPerformanceCounter();
    float elapsed_ms =
      (end_ts - start_ts) / (float)SDL_GetPerformanceFrequency() * 1000.0f;

    // Limit to 60 FPS
    SDL_Delay(std::max(0, (int)floor(16.666f - elapsed_ms)));
  }

  simuStop();

  SDL_DestroyTexture(screen_texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_CloseAudio();
  SDL_Quit();

  return 0;
}

// WASM imports required by simu driver stubs
// TODO: point to real hardware
// or to linux devices (device tree)
uint16_t simuGetAnalog(uint8_t idx)
{
  auto max_sticks = adcGetMaxInputs(ADC_INPUT_MAIN);
  if (idx < max_sticks) {
    // Return center position for all gimbal axes
    switch (idx){
      case 0:return 2048;
      case 1:return 2048;
      case 2:return 2048;
      case 3:return 2048;
    }
  }

  idx -= max_sticks;

  auto max_pots = adcGetMaxInputs(ADC_INPUT_FLEX);
  if (idx < max_pots) {
    // Return center position for all pots/sliders
    switch (getPotType(idx)){
      case FLEX_POT:
      case FLEX_POT_CENTER:
      case FLEX_SLIDER:
        return 2048;
      case FLEX_MULTIPOS:
        return 4096/3; 
    }
  }
  return 0;
}

void simuTrace(const char* text) {}

void simuLcdNotify() {}

void simuMinimize()
{
  SDL_MinimizeWindow(window);
}

void simuShutdown()
{
  app_running = false;
}
