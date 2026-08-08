#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int height;
  int width;
  int pitch;

  uint32_t *pixels;
  XImage *image;

} Framebuffer;

static void render(Framebuffer *buffer) {
  for (int y = 0; y < buffer->height; y++) {
    for (int x = 0; x < buffer->width; x++) {
      uint8_t red = (uint8_t)(x * 255 / buffer->width);
      uint8_t green = (uint8_t)(y * 255 / buffer->height);
      uint8_t blue = 128;

      uint32_t pixel =
          ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;

      buffer->pixels[y * buffer->width + x] = pixel;
    }
  }
}

static int resize_framebuffer(Display *display, int screen, Framebuffer *buffer,
                              int width, int height) {
  if (buffer->image) {
    XDestroyImage(buffer->image);
    buffer->image = NULL;
    buffer->pixels = NULL;
  }

  buffer->width = width;
  buffer->height = height;
  buffer->pitch = width * sizeof(uint32_t);

  buffer->pixels =
      malloc((size_t)buffer->width * (size_t)buffer->height * sizeof(uint32_t));

  if (!buffer->pixels) {
    fprintf(stderr, "Could not allocate framebuffer\n");
    return 0;
  }

  buffer->image = XCreateImage(display, DefaultVisual(display, screen),
                               DefaultDepth(display, screen), ZPixmap, 0,
                               (char *)buffer->pixels, buffer->width,
                               buffer->height, 32, buffer->pitch);

  if (!buffer->image) {
    fprintf(stderr, "Could not create XImage\n");
    free(buffer->pixels);
    buffer->pixels = NULL;
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

  // printf("Screen: %dx%d\n", DisplayWidth(display, screen),
  //       DisplayHeight(display, screen));

  XStoreName(display, window, "Handmade");

  XSelectInput(display, window,
               ExposureMask | KeyPressMask | KeyReleaseMask |
                   StructureNotifyMask);

  XMapWindow(display, window);

  Framebuffer buffer = {0};

  if (!resize_framebuffer(display, screen, &buffer, width, height)) {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 1;
  }

  render(&buffer);

  int running = 1;

  while (running) {
    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      if (event.type == KeyPress) {
        running = 0;
      }
      if (event.type == ConfigureNotify) {

        int new_width = event.xconfigure.width;
        int new_height = event.xconfigure.height;

        if (new_width != buffer.width || new_height != buffer.height) {
          resize_framebuffer(display, screen, &buffer, new_width, new_height);
        }
        render(&buffer);
      }
    }
    XPutImage(display, window, DefaultGC(display, screen), buffer.image, 0, 0,
              0, 0, buffer.width, buffer.height);
  }

  /*
  for (;;) {
    XEvent event;
    XNextEvent(display, &event);

    if (event.type == Expose) {
      XClearWindow(display, window);
    }

    if (event.type == KeyPress) {
      break;
    }
  }
  */
  XDestroyImage(buffer.image);

  XDestroyWindow(display, window);
  XCloseDisplay(display);

  return 0;
}
