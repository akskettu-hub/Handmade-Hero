#include "handmade.h"
#include <math.h> // TODO: self implement Pi and sin
#include <stdint.h>

void render(GameRenderBuffer *buffer, GameGradientOffsets *offsets) {
  for (int y = 0; y < buffer->height; y++) {
    for (int x = 0; x < buffer->width; x++) {
      uint8_t red = (uint8_t)(x * 255 / buffer->width + offsets->xOffset);
      uint8_t green = (uint8_t)(y * 255 / buffer->height + offsets->yOffset);
      uint8_t blue = 128;

      uint32_t pixel =
          ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;

      buffer->pixels[y * buffer->width + x] = pixel;
    }
  }
}

void gameAudioGenerate(GameAudioState *audio, int frames) {
  int16_t *pFrame = audio->buffer;
  for (int frame = 0; frame < frames; frame++) {
    double value = sin(audio->phase * 2.0 * M_PI);

    int16_t sample = (int16_t)(value * audio->toneVolume);

    *pFrame++ = sample;
    *pFrame++ = sample;

    audio->phase += audio->frequency / audio->sampleRate;

    if (audio->phase >= 1.0) {
      audio->phase -= 1.0;
    }
  }
}

void gameControlGradientOffset(GameInput *input, GameGradientOffsets *offsets) {
  if (input->left) {
    offsets->xOffset += input->left;
  }

  if (input->right) {
    offsets->xOffset -= input->right;
  }

  if (input->up) {
    offsets->yOffset += input->up;
  }

  if (input->down) {
    offsets->yOffset -= input->down;
  }
}

void gameControlSineFrequency(GameInput *input, GameAudioState *audio) {
  if (input->pitchUp) {
    audio->frequency += input->pitchUp;

    if (audio->frequency > 880.0) {
      audio->frequency = 880.0;
    }
  }

  if (input->pitchDown) {
    audio->frequency -= input->pitchDown;

    if (audio->frequency < 220.0) {
      audio->frequency = 220.0;
    }
  }
}
