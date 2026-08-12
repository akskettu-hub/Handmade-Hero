#ifndef HANDMADE_H
#define HANDMADE_H

#include <stdint.h>

typedef struct GameRenderBuffer {
  int height;
  int width;
  int pitch;

  uint32_t *pixels;
} GameRenderBuffer;

void render(GameRenderBuffer *buffer, uint8_t y_offset, uint8_t x_offset);

typedef struct {
  int up;
  int down;
  int left;
  int right;
} GameInput;

#endif
