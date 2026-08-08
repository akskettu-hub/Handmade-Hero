// main thing
//
#include <X11/Xlib.h>
#include <stdio.h>

int main(void) {
  Display *display = XOpenDisplay(NULL); // Establish connection to X server

  if (!display) {
    fprintf(stderr, "Could not open X display\n");
    return 1;
  }

  int screen = DefaultScreen(display); // Figure out which srcreen is used

  int screen_width = DisplayWidth(display, screen);
  int screen_height = DisplayHeight(display, screen);

  int window_width = screen_width / 2;
  int window_height = screen_height / 2;

  Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 100,
                                      100, window_width, window_height, 1,
                                      BlackPixel(display, screen),
                                      WhitePixel(display, screen));

  // printf("Screen: %dx%d\n", DisplayWidth(display, screen),
  //       DisplayHeight(display, screen));

  XStoreName(display, window, "Handmade");

  XSelectInput(display, window,
               ExposureMask | KeyPressMask | KeyReleaseMask |
                   StructureNotifyMask);

  XMapWindow(display, window);

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

  XDestroyWindow(display, window);
  XCloseDisplay(display);

  return 0;
}
