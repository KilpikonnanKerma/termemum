
#ifndef _WIN_H
#define _WIN_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long

typedef struct {
    ushort red;
    ushort green;
    ushort blue;
    ushort alpha;
} Color;

typedef struct {
    int width;
    int height;
    int cols;
    int rows;
    int char_width;
    int char_height;
} Wnd;

extern Wnd window;

extern XftFont *xftFont;
extern XftDraw *draw;
extern Display *dpy;
extern Visual *visual;
extern Colormap colormap;
extern Window win;

#endif
