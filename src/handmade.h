#ifndef HANDMADE_H
#define HANDMADE_H

#include "platform_audio.h"
#include <stdint.h>

// NOTE: Services that the game provides to the platform layer.
// There should be no platform specific code in here.

typedef struct GameRenderBuffer {
  int height;
  int width;
  int pitch;

  uint32_t *pixels;
} GameRenderBuffer;

void render(GameRenderBuffer *buffer, uint8_t y_offset, uint8_t x_offset);

typedef struct {
  int16_t *buffer;

  int sample_rate;
  int channels;

  double phase;
  double frequency;
  int16_t toneVolume;
} GameAudioState;

void gameAudioGenerate(GameAudioState *audio, int frames);

// NOTE: Services that the platform layer provides to the game.

typedef struct {
  int up;
  int down;
  int left;
  int right;
} GameInput;

#endif
