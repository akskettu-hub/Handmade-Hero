#ifndef HANDMADE_H
#define HANDMADE_H

#include <X11/Xlib.h>
#include <stdint.h>

typedef struct {
  int height;
  int width;
  int pitch;

  uint32_t *pixels;
  XImage *image;
} Framebuffer;

static void render(Framebuffer *buffer, uint8_t y_offset, uint8_t x_offset);

typedef struct {
  int up;
  int down;
  int left;
  int right;
} GameInput;

#endif
