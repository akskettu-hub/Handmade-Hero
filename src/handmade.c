#include "handmade.h"

static void render(Framebuffer *buffer, uint8_t y_offset, uint8_t x_offset) {
  for (int y = 0; y < buffer->height; y++) {
    for (int x = 0; x < buffer->width; x++) {
      uint8_t red = (uint8_t)(x * 255 / buffer->width + x_offset);
      uint8_t green = (uint8_t)(y * 255 / buffer->height + y_offset);
      uint8_t blue = 128;

      uint32_t pixel =
          ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;

      buffer->pixels[y * buffer->width + x] = pixel;
    }
  }
}
