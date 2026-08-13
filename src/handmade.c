#include "handmade.h"
#include <math.h>

void render(GameRenderBuffer *buffer, uint8_t y_offset, uint8_t x_offset) {
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

void gameAudioGenerate(GameAudioState *audio, int16_t *buffer, int frames) {
  for (int frame = 0; frame < frames; frame++) {
    double value = sin(audio->phase * 2.0 * M_PI);

    int16_t sample = (int16_t)(value * audio->toneVolume);

    buffer[frame * 2 + 0] = sample;
    buffer[frame * 2 + 1] = sample;

    audio->phase += audio->frequency / audio->sample_rate;

    if (audio->phase >= 1.0) {
      audio->phase -= 1.0;
    }
  }
}
