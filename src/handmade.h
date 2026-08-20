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

typedef struct {
  uint8_t xOffset;
  uint8_t yOffset;
} GameGradientOffsets;

void render(GameRenderBuffer *buffer, GameGradientOffsets *offsets);

typedef struct {
  int16_t *buffer;

  int sampleRate;
  int channels;

  double phase;
  double frequency;
  int16_t toneVolume;
} GameAudioState;

void gameAudioGenerate(GameAudioState *audio, int frames);

void DEBUGFileIOTest(void); // test of debug IO

// NOTE: Services that the platform layer provides to the game.

#if HANDMADE_INTERNAL
typedef struct {
  size_t contentsSize;
  void *contents;
} DEBUGReadFileResult;

DEBUGReadFileResult DEBUGPlatformReadEntireFile(char *filename);
void DEBUGPlatformFreeFileMemory(void *memory);

uint8_t DEBUGPlatformWriteEntireFile(char *filename, uint32_t memorySize,
                                     void *memory);
#endif

typedef struct {
  int up;
  int down;
  int left;
  int right;

  int pitchUp;
  int pitchDown;
} GameInput;

void gameControlGradientOffset(GameInput *input, GameGradientOffsets *offsets);

void gameControlSineFrequency(GameInput *input, GameAudioState *audio);

#endif
