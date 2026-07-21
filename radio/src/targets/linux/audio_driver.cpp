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

#include "audio.h"
#include "linux_simulib.h"

#if !defined(SOFTWARE_VOLUME)
static int volume = 0;

void audioSetVolume(uint8_t _volume)
{
  volume = _volume;
}
#endif

int AudioGetVolume()
{
#if !defined(SOFTWARE_VOLUME)
  return volume;
#else
  return VOLUME_LEVEL_MAX;
#endif
}

void audioConsumeCurrentBuffer()
{
  auto& fifo = audioQueue.buffersFifo;
  while(true) {
    auto nextBuffer = fifo.getNextFilledBuffer();
    if (!nextBuffer) return;

#if !defined(SOFTWARE_VOLUME)
    int volume = AudioGetVolume();
    if (volume < VOLUME_LEVEL_MAX) {
      auto* buf = const_cast<AudioBuffer*>(nextBuffer);
      for (uint16_t i = 0; i < buf->size; ++i) {
        buf->data[i] = (audio_data_t)(
            ((int32_t)buf->data[i] * volume) / VOLUME_LEVEL_MAX);
      }
    }
#endif

    auto data = (const uint8_t*)nextBuffer->data;
    uint32_t len = nextBuffer->size * sizeof(audio_data_t);
    QueueAudio(data, len);
    fifo.freeNextFilledBuffer();
  }
}

bool simuAudioInit() { return AudioInit(); }
void simuAudioDeInit() { AudioDeInit(); }
int simuAudioGetVolume() { return AudioGetVolume(); }
