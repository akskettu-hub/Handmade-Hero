
#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <bits/time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "handmade.h"
#include "platform_audio.h"

typedef struct {
  uint32_t *pixels;
  XImage *image;
} LinuxImage;

typedef struct {
  GameRenderBuffer gameBuffer;
  XImage *image;
} LinuxFramebuffer;

static int linuxResizeGamebuffer(Display *display, int screen,
                                 LinuxFramebuffer *buffer, int width,
                                 int height) {
  if (buffer->image) {
    XDestroyImage(buffer->image);
    buffer->image = NULL;
    buffer->gameBuffer.pixels = NULL;
  }

  buffer->gameBuffer.width = width;
  buffer->gameBuffer.height = height;
  buffer->gameBuffer.pitch = width * sizeof(uint32_t);

  buffer->gameBuffer.pixels =
      malloc((size_t)buffer->gameBuffer.width *
             (size_t)buffer->gameBuffer.height * sizeof(uint32_t));

  if (!buffer->gameBuffer.pixels) {
    fprintf(stderr, "Could not allocate framebuffer\n");
    return 0;
  }

  buffer->image = XCreateImage(
      display, DefaultVisual(display, screen), DefaultDepth(display, screen),
      ZPixmap, 0, (char *)buffer->gameBuffer.pixels, buffer->gameBuffer.width,
      buffer->gameBuffer.height, 32, buffer->gameBuffer.pitch);

  if (!buffer->image) {
    fprintf(stderr, "Could not create XImage\n");
    free(buffer->gameBuffer.pixels);
    buffer->gameBuffer.pixels = NULL;
    return 0;
  }
  return 1;
}

DEBUGReadFileResult DEBUGPlatformReadEntireFile(char *filename) {
  DEBUGReadFileResult result = {0};

  FILE *file = fopen(filename, "rb");
  if (!file) {
    return result;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return result;
  }

  long fileSize = ftell(file);
  if (fileSize < 0) {
    fclose(file);
    return result;
  }

  rewind(file);

  void *contents = malloc((size_t)fileSize);
  if (!contents) {
    fclose(file);
    return result;
  }

  size_t bytesRead = fread(contents, 1, (size_t)fileSize, file);

  fclose(file);

  if (bytesRead != (size_t)fileSize) {
    free(contents);
    return result;
  }
  result.contents = contents;
  result.contentsSize = bytesRead;

  return result;
}
void DEBUGPlatformFreeFileMemory(void *memory) { free(memory); }

// uint8_t DEBUGPlatformWriteEntireFile(char *filename, uint32_t memorySize,
// void *memory) {}

int main(void) {
  Display *display = XOpenDisplay(NULL); // Establish connection to X server

  if (!display) {
    fprintf(stderr, "Could not open X display\n");
    return 1;
  }

  Bool detectableAutoRepeat;

  if (!XkbSetDetectableAutoRepeat(display, True, &detectableAutoRepeat)) {
    fprintf(stderr, "Could not detect auto repeat.\n");
  }

  // Init audio
  AudioState *audio = audioInit();

  if (!audio) {
    fprintf(stderr, "Could not initialise audio\n");
    return 1;
  }

  int16_t tempBuffer[AUDIO_GENERATE_BUFFER_FRAMES * 2];
  int16_t tempOutputBuffer[AUDIO_OUTPUT_BUFFER_FRAMES * 2];

  GameAudioState gameAudioState = {0};
  gameAudioState.buffer = tempBuffer;
  gameAudioState.sampleRate = AUDIO_SAMPLE_RATE;
  gameAudioState.channels = AUDIO_CHANNELS;
  gameAudioState.phase = 0.0;
  gameAudioState.frequency = 440.0;
  gameAudioState.toneVolume = 3000;

  // int16_t audio_buffer[AUDIO_FRAMES * 2];
  //  printf("size of audio buffer: %ld\n", sizeof(audio_buffer));

  // End of init audio

  int screen = DefaultScreen(display); // Figure out which srcreen is used

  int screenWidth = DisplayWidth(display, screen);
  int screenHeight = DisplayHeight(display, screen);

  int width = screenWidth / 2;
  int height = screenHeight / 2;

  Window window = XCreateSimpleWindow(
      display, RootWindow(display, screen), 100, 100, width, height, 1,
      BlackPixel(display, screen), WhitePixel(display, screen));

  XStoreName(display, window, "Handmade");

  XSelectInput(display, window,
               ExposureMask | KeyPressMask | KeyReleaseMask |
                   StructureNotifyMask);

  XMapWindow(display, window);

  LinuxFramebuffer buffer = {0};

  if (!linuxResizeGamebuffer(display, screen, &buffer, width, height)) {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 1;
  }

  GameInput gameInput;

  GameGradientOffsets gradientOffsets = {0};

  render(&buffer.gameBuffer, &gradientOffsets);

  struct timespec clockRes;
  clock_getres(CLOCK_MONOTONIC, &clockRes);
  printf("Clock res: %ld s, %ld ns\n", clockRes.tv_sec, clockRes.tv_nsec);

  struct timespec counter;
  struct timespec prevCounter;
  clock_gettime(CLOCK_MONOTONIC, &prevCounter);

  int running = 1;
  while (running) {
    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      if (event.type == KeyPress) {
        KeySym key = XLookupKeysym(&event.xkey, 0);

        if (key == XK_Escape) {
          running = 0;
        }

        if (key == XK_w) {
          gameInput.up = 1;
          printf("gameInput.up = %d\n", gameInput.up);
        }

        if (key == XK_s) {
          gameInput.down = 1;
          printf("gameInput.down = %d\n", gameInput.down);
        }

        if (key == XK_a) {
          gameInput.left = 1;
          printf("gameInput.left = %d\n", gameInput.left);
        }

        if (key == XK_d) {
          gameInput.right = 1;
          printf("gameInput.right = %d\n", gameInput.right);
        }

        if (key == XK_t) {
          gameInput.pitchUp = 1;
          printf("gameInput.pitchUp = %d\n", gameInput.pitchUp);
        }

        if (key == XK_g) {
          gameInput.pitchDown = 1;
          printf("gameInput.pitchDown = %d\n", gameInput.right);
        }
      }

      if (event.type == KeyRelease) {
        KeySym key = XLookupKeysym(&event.xkey, 0);

        if (key == XK_w) {
          gameInput.up = 0;
          printf("gameInput.up = %d\n", gameInput.up);
        }

        if (key == XK_s) {
          gameInput.down = 0;
          printf("gameInput.down = %d\n", gameInput.down);
        }

        if (key == XK_a) {
          gameInput.left = 0;
          printf("gameInput.left = %d\n", gameInput.left);
        }

        if (key == XK_d) {
          gameInput.right = 0;
          printf("gameInput.right = %d\n", gameInput.right);
        }

        if (key == XK_t) {
          gameInput.pitchUp = 0;
          printf("gameInput.pitchUp = %d\n", gameInput.pitchUp);
        }

        if (key == XK_g) {
          gameInput.pitchDown = 0;
          printf("gameInput.pitchDown = %d\n", gameInput.right);
        }
      }

      if (event.type == ConfigureNotify) {
        int newWidth = event.xconfigure.width;
        int newHeight = event.xconfigure.height;

        if (newWidth != buffer.gameBuffer.width ||
            newHeight != buffer.gameBuffer.height) {
          linuxResizeGamebuffer(display, screen, &buffer, newWidth, newHeight);
        }
      }
    }
    gameControlGradientOffset(&gameInput, &gradientOffsets);

    render(&buffer.gameBuffer, &gradientOffsets);

    XPutImage(display, window, DefaultGC(display, screen), buffer.image, 0, 0,
              0, 0, buffer.gameBuffer.width, buffer.gameBuffer.height);

    // audio

    // struct timespec tic;
    // struct timespec toc;

    // clock_gettime(CLOCK_MONOTONIC, &tic);
    gameControlSineFrequency(&gameInput, &gameAudioState);
    int framesToGenerate = linuxAudioRequestedFrames(audio);
    gameAudioGenerate(&gameAudioState, framesToGenerate);

    audioUpdate(audio, tempBuffer, tempOutputBuffer, framesToGenerate);
    // clock_gettime(CLOCK_MONOTONIC, &toc);

    // long long elapsed = (toc.tv_sec - tic.tv_sec) * 1000000000LL +
    // (toc.tv_nsec - tic.tv_nsec);

    // printf("Audio time: %lld ns\n", elapsed);
    //  end of audio

    clock_gettime(CLOCK_MONOTONIC, &counter);
    long long elapsed = (counter.tv_sec - prevCounter.tv_sec) * 1000000000LL +
                        (counter.tv_nsec - prevCounter.tv_nsec);
    prevCounter = counter;

    long long fps = 1000000000LL / elapsed;

    printf("Elapsed: %lld ns, %lld FPS\n", elapsed, fps);
  }
  // NOTE: File I/O test.
  DEBUGReadFileResult file = DEBUGPlatformReadEntireFile("data/io_test.txt");

  if (file.contents) {
    printf("test debug file io. Size: %ldB\n", file.contentsSize);
    DEBUGPlatformFreeFileMemory(file.contents);
  } else {
    printf("no file\n");
  }
  audioShutdown(audio);

  XDestroyImage(buffer.image);

  XDestroyWindow(display, window);
  XCloseDisplay(display);

  printf("Exited nicely :)\n");

  return 0;
}
