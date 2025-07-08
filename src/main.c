#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <vterm.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pty.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

#include "input.h"

#define FONT_WIDTH 10
#define FONT_HEIGHT 16

// Callback to send data from libvterm back to shell
int write_to_pty(const char *s, long unsigned int len, void *user) {
    int *pty_fd = (int *)user;
    return write(*pty_fd, s, len);
}

unsigned short convert_color_component(uint8_t val) {
	return (unsigned short)(val * 257);
}

XftColor xft_color_from_vterm(Display *dpy, Visual *visual, Colormap colormap, VTermColor *vtcolor) {
    XRenderColor xrcolor;
    xrcolor.red   = convert_color_component(vtcolor->rgb.red);
    xrcolor.green = convert_color_component(vtcolor->rgb.green);
    xrcolor.blue  = convert_color_component(vtcolor->rgb.blue);
    xrcolor.alpha = 0xFFFF;

    XftColor xftcolor;
    XftColorAllocValue(dpy, visual, colormap, &xrcolor, &xftcolor);
    return xftcolor;
}

int main() {

    // Initial window settings
    int WIDTH = 800;
    int HEIGHT = 600;
    int COLS = WIDTH / 10;
    int ROWS = HEIGHT / 16;

    // Xlib init
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;
    int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	Visual *visual = DefaultVisual(dpy, screen);
	Colormap colormap = XCreateColormap(dpy, root, visual, AllocNone);

	XSetWindowAttributes attrs;
	attrs.colormap = colormap;
	attrs.background_pixel = BlackPixel(dpy, screen);
	attrs.border_pixel = WhitePixel(dpy, screen);
	attrs.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

	unsigned long valuemask = CWColormap | CWBackPixel | CWBorderPixel | CWEventMask;

    Window win = XCreateWindow(dpy, root, 100, 100,
			WIDTH, HEIGHT, 1, DefaultDepth(dpy, screen),
			InputOutput, visual, valuemask, &attrs);
    XMapWindow(dpy, win);

    GC gc = XCreateGC(dpy, win, 0, NULL);
    XFontStruct *font = XLoadQueryFont(dpy, "fixed");
    if (!font) return 1;
    XSetFont(dpy, gc, font->fid);

    XftFont *xftFont = XftFontOpenName(dpy, screen, "monospace-14");
    XftDraw *draw = XftDrawCreate(dpy, win, visual, colormap);

    // PTY + shell
    int masterFd;
    pid_t pid = forkpty(&masterFd, NULL, NULL, NULL);
    if (pid == 0) {
        execlp("bash", "bash", NULL);
        exit(1);
    }

    // libvterm init
    VTerm *vt = vterm_new(ROWS, COLS);
    vterm_set_utf8(vt, 1);
    vterm_output_set_callback(vt, (void*)write_to_pty, &masterFd);
    VTermScreen *vtermScreen = vterm_obtain_screen(vt);
    vterm_screen_reset(vtermScreen, 1);

    VTermState *vtermState = vterm_obtain_state(vt);

    char buf[1024];

    XftColor xft_bg;
    XRenderColor bg_color = {
        .red = 0,
        .green = 0,
        .blue = 0,
        .alpha = 0xFFFF,
    };
    XftColorAllocValue(dpy, visual, colormap, &bg_color, &xft_bg);

    XftColor xft_fg;
    XRenderColor fg_color = {
        .red = convert_color_component(255),
        .green = convert_color_component(255),
        .blue = convert_color_component(255),
        .alpha = 0xFFFF,
    };

    XftColorAllocValue(dpy, visual, colormap, &fg_color, &xft_fg);


    XftColor xftCursor;
    XRenderColor cursorColor = {
        .red = 0xFFFF,
        .green = 0xFFFF,
        .blue = 0xFFFF,
        .alpha = 0xFFFF,
    };
    XftColorAllocValue(dpy, visual, colormap, &cursorColor, &xftCursor);



    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(masterFd, &fds);
        FD_SET(ConnectionNumber(dpy), &fds);

        int maxfd = (masterFd > ConnectionNumber(dpy)) ? masterFd : ConnectionNumber(dpy);
        if (select(maxfd + 1, &fds, NULL, NULL, NULL) < 0) break;

        // Shell output
        if (FD_ISSET(masterFd, &fds)) {
            int len = read(masterFd, buf, sizeof(buf));
            if (len > 0) {
                vterm_input_write(vt, buf, len);
                vterm_screen_flush_damage(vtermScreen);                
            }
        }

        // Keypress input
        if (FD_ISSET(ConnectionNumber(dpy), &fds)) {
            XEvent ev;
            while(XPending(dpy)) {
                XNextEvent(dpy, &ev);
                if (ev.type == KeyPress) {
                    handle_keypress(&ev.xkey, vt, masterFd);
                } else if (ev.type == ConfigureNotify) {

                    int new_width = ev.xconfigure.width;
                    int new_height = ev.xconfigure.height;
                    int new_cols = new_width / FONT_WIDTH;
                    int new_rows = new_height / FONT_HEIGHT;

                    if (new_cols != COLS || new_rows != ROWS) {
                        WIDTH = new_width;
                        HEIGHT = new_height;
                        COLS = new_cols;
                        ROWS = new_rows;
                        vterm_set_size(vt, ROWS, COLS);
                        XftDrawChange(draw, win);
                    }
                }
            }
        }

        // Draw screen
        // XClearWindow(dpy, win);
        XftDrawRect(draw, &xft_bg, 0, 0, WIDTH, HEIGHT);
        VTermScreenCell cell;
        VTermPos pos;

        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                pos.row = row;
                pos.col = col;
                if (!vterm_screen_get_cell(vtermScreen, pos, &cell)) continue;
                if (cell.chars[0] == 0) continue;

                XftColor xft_fg_cell = xft_color_from_vterm(dpy, visual, colormap, &cell.fg);

                XSetForeground(dpy, gc, WhitePixel(dpy, screen));

                // XftDrawStringUtf8(draw, &xft_fg, xftFont, col * FONT_WIDTH, (row + 1) * xftFont->height, (FcChar8*)cell.chars , strlen((char*)cell.chars));

                XftDrawStringUtf8(draw, &xft_fg_cell, xftFont,
                    col * FONT_WIDTH,
                    (row + 1) * xftFont->height,
                    (FcChar8*)cell.chars,
                    strlen((char*)cell.chars));

                XftColorFree(dpy, visual, colormap, &xft_fg_cell);
            }
        }

        VTermPos cursorPos;
        vterm_state_get_cursorpos(vtermState, &cursorPos);

        XftDrawRect(draw, &xftCursor, cursorPos.col * FONT_WIDTH, (cursorPos.row + 1) * xftFont->height - FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT);

        XFlush(dpy);
    }

	XFreeColormap(dpy, colormap);
    XftColorFree(dpy, visual, colormap, &xftCursor);
	vterm_free(vt);
	XCloseDisplay(dpy);
	return 0;
}
