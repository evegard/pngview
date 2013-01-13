#include <stdlib.h>
#include <X11/Xlib.h>

#include "png.h"

void gui_display_image(png_t *png)
{
    Display *display = XOpenDisplay(NULL);
    int screen = DefaultScreen(display);
    int black = BlackPixel(display, screen);
    Window window = XCreateSimpleWindow(
        display,
        DefaultRootWindow(display),
        0, 0,
        png->width, png->height,
        0,
        black, black);
    XMapWindow(display, window);

    GC gc = DefaultGC(display, screen);
    Visual *visual = DefaultVisual(display, screen);

    char *data = malloc(png->width * png->height * 4);
    for (int row = 0; row < png->height; row++) {
        for (int col = 0; col < png->width; col++) {
            data[(row * png->width + col) * 4] =
                png->data[(row * png->width + col) * 3 + 2];
            data[(row * png->width + col) * 4 + 1] =
                png->data[(row * png->width + col) * 3 + 1];
            data[(row * png->width + col) * 4 + 2] =
                png->data[(row * png->width + col) * 3];
            data[(row * png->width + col) * 4 + 3] = 0;
        }
    }

    XImage *image = XCreateImage(display, visual, 24, ZPixmap, 0,
        data, png->width, png->height, 32, 0);

    XSelectInput(display, window, ExposureMask);

    while (1) {
        XEvent event;
        XNextEvent(display, &event);
        
        switch(event.type) {
            case Expose:
                if (event.xexpose.count > 0) {
                    break;
                }

                printf("expose event\n");

                XPutImage(display, window, gc, image, 0, 0, 0, 0,
                    png->width, png->height);
                break;
        }
    }

    XCloseDisplay(display);
}
