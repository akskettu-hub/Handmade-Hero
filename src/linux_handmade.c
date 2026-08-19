
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
  GameRenderBuffer game_buffer;
  XImage *image;
} LinuxFramebuffer;

static int linux_resize_gamebuffer(Display *display, int screen,
                                   LinuxFramebuffer *buffer, int width,
                                   int height) {
  if (buffer->image) {
    XDestroyImage(buffer->image);
    buffer->image = NULL;
    buffer->game_buffer.pixels = NULL;
  }

  buffer->game_buffer.width = width;
  buffer->game_buffer.height = height;
  buffer->game_buffer.pitch = width * sizeof(uint32_t);

  buffer->game_buffer.pixels =
      malloc((size_t)buffer->game_buffer.width *
             (size_t)buffer->game_buffer.height * sizeof(uint32_t));

  if (!buffer->game_buffer.pixels) {
    fprintf(stderr, "Could not allocate framebuffer\n");
    return 0;
  }

  buffer->image = XCreateImage(
      display, DefaultVisual(display, screen), DefaultDepth(display, screen),
      ZPixmap, 0, (char *)buffer->game_buffer.pixels, buffer->game_buffer.width,
      buffer->game_buffer.height, 32, buffer->game_buffer.pitch);

  if (!buffer->image) {
    fprintf(stderr, "Could not create XImage\n");
    free(buffer->game_buffer.pixels);
    buffer->game_buffer.pixels = NULL;
    return 0;
  }
  return 1;
}

int main(void) {
  Display *display = XOpenDisplay(NULL); // Establish connection to X server

  if (!display) {
    fprintf(stderr, "Could not open X display\n");
    return 1;
  }

  Bool detectable_auto_repeat;

  if (!XkbSetDetectableAutoRepeat(display, True, &detectable_auto_repeat)) {
    fprintf(stderr, "Could not detect auto repeat.\n");
  }

  // Init audio
  AudioState *audio = audio_init();

  if (!audio) {
    fprintf(stderr, "Could not initialise audio\n");
    return 1;
  }

  int16_t temp_buffer[AUDIO_GENERATE_BUFFER_FRAMES * 2];
  int16_t temp_output_buffer[AUDIO_OUTPUT_BUFFER_FRAMES * 2];

  GameAudioState gameAudioState = {0};
  gameAudioState.buffer = temp_buffer;
  gameAudioState.sample_rate = AUDIO_SAMPLE_RATE;
  gameAudioState.channels = AUDIO_CHANNELS;
  gameAudioState.phase = 0.0;
  gameAudioState.frequency = 440.0;
  gameAudioState.toneVolume = 3000;

  // int16_t audio_buffer[AUDIO_FRAMES * 2];
  //  printf("size of audio buffer: %ld\n", sizeof(audio_buffer));

  // End of init audio

  int screen = DefaultScreen(display); // Figure out which srcreen is used

  int screen_width = DisplayWidth(display, screen);
  int screen_height = DisplayHeight(display, screen);

  int width = screen_width / 2;
  int height = screen_height / 2;

  // int window_width = 960;
  // int window_height = 540;

  Window window = XCreateSimpleWindow(
      display, RootWindow(display, screen), 100, 100, width, height, 1,
      BlackPixel(display, screen), WhitePixel(display, screen));

  XStoreName(display, window, "Handmade");

  XSelectInput(display, window,
               ExposureMask | KeyPressMask | KeyReleaseMask |
                   StructureNotifyMask);

  XMapWindow(display, window);

  LinuxFramebuffer buffer = {0};

  if (!linux_resize_gamebuffer(display, screen, &buffer, width, height)) {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 1;
  }

  GameInput game_input;

  GameGradientOffsets gradientOffsets = {0};

  render(&buffer.game_buffer, &gradientOffsets);

  struct timespec clock_res;
  clock_getres(CLOCK_MONOTONIC, &clock_res);
  printf("Clock res: %ld s, %ld ns\n", clock_res.tv_sec, clock_res.tv_nsec);

  struct timespec counter;
  struct timespec prev_counter;
  clock_gettime(CLOCK_MONOTONIC, &prev_counter);

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
          game_input.up = 1;
          printf("game_input.up = %d\n", game_input.up);
        }

        if (key == XK_s) {
          game_input.down = 1;
          printf("game_input.down = %d\n", game_input.down);
        }

        if (key == XK_a) {
          game_input.left = 1;
          printf("game_input.left = %d\n", game_input.left);
        }

        if (key == XK_d) {
          game_input.right = 1;
          printf("game_input.right = %d\n", game_input.right);
        }
      }

      if (event.type == KeyRelease) {
        KeySym key = XLookupKeysym(&event.xkey, 0);

        if (key == XK_w) {
          game_input.up = 0;
          printf("game_input.up = %d\n", game_input.up);
        }

        if (key == XK_s) {
          game_input.down = 0;
          printf("game_input.down = %d\n", game_input.down);
        }

        if (key == XK_a) {
          game_input.left = 0;
          printf("game_input.left = %d\n", game_input.left);
        }

        if (key == XK_d) {
          game_input.right = 0;
          printf("game_input.right = %d\n", game_input.right);
        }
      }

      if (event.type == ConfigureNotify) {
        int new_width = event.xconfigure.width;
        int new_height = event.xconfigure.height;

        if (new_width != buffer.game_buffer.width ||
            new_height != buffer.game_buffer.height) {
          linux_resize_gamebuffer(display, screen, &buffer, new_width,
                                  new_height);
        }
      }
    }
    gameControlGradientOffset(&game_input, &gradientOffsets);

    render(&buffer.game_buffer, &gradientOffsets);

    XPutImage(display, window, DefaultGC(display, screen), buffer.image, 0, 0,
              0, 0, buffer.game_buffer.width, buffer.game_buffer.height);

    // y_offset++;
    // x_offset++;

    // audio
    // NOTE: Refactor audio so that generation happens on geme layer
    // - platform layer figures out how many frames are needed
    // - Asks game to generate that amount (ideally to the ring buffer)
    // - Platform writes generated into ring
    // - Platform reads from ring int pcm buffer

    // struct timespec tic;
    // struct timespec toc;

    // clock_gettime(CLOCK_MONOTONIC, &tic);
    int framesToGenerate = linuxAudioRequestedFrames(audio);

    gameAudioGenerate(&gameAudioState, framesToGenerate);
    audio_update(audio, temp_buffer, temp_output_buffer, framesToGenerate);
    // clock_gettime(CLOCK_MONOTONIC, &toc);

    // long long elapsed = (toc.tv_sec - tic.tv_sec) * 1000000000LL +
    // (toc.tv_nsec - tic.tv_nsec);

    // printf("Audio time: %lld ns\n", elapsed);
    //  end of audio

    clock_gettime(CLOCK_MONOTONIC, &counter);
    long long elapsed = (counter.tv_sec - prev_counter.tv_sec) * 1000000000LL +
                        (counter.tv_nsec - prev_counter.tv_nsec);
    prev_counter = counter;

    long long fps = 1000000000LL / elapsed;

    printf("Elapsed: %lld ns, %lld FPS\n", elapsed, fps);
  }
  audio_shutdown(audio);

  XDestroyImage(buffer.image);

  XDestroyWindow(display, window);
  XCloseDisplay(display);

  printf("Exited nicely :)\n");

  return 0;
}
