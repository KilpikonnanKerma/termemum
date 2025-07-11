#include <vterm.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pty.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

#include "win.h"
#include "input.h"
#include "config.h"

XftFont *xftFont;
XftDraw *draw;
Display *dpy;
Visual *visual;
Colormap colormap;
Window win;

Wnd window;

XftColor color_cache[MAX_COLORS];
bool color_cached[MAX_COLORS] = {false};

// Callback to send data from libvterm back to shell
int write_to_pty(const char *s, long unsigned int len, void *user) {
    int *pty_fd = (int *)user;
    return write(*pty_fd, s, len);
}

unsigned short convert_color_component(uint8_t val) {
	return (unsigned short)(val * 257);
}

int get_color_index(VTermColor *c) {
    return (c->rgb.red * 31 + c->rgb.green * 17 + c->rgb.blue * 13) % MAX_COLORS;
}

XftColor xft_color_from_vterm(Display *dpy, Visual *visual, Colormap colormap, VTermColor *vtcolor) {
    int idx = get_color_index(vtcolor);
    if (color_cached[idx]) return color_cache[idx];

    XRenderColor xrcolor = {
        .red   = convert_color_component(vtcolor->rgb.red),
        .green = convert_color_component(vtcolor->rgb.green),
        .blue  = convert_color_component(vtcolor->rgb.blue),
        .alpha = 0xFFFF
    };

    if (!XftColorAllocValue(dpy, visual, colormap, &xrcolor, &color_cache[idx])) {
        xrcolor.red = xrcolor.green = xrcolor.blue = 0xFFFF;
        color_cache[idx].pixel = WhitePixel(dpy, DefaultScreen(dpy));
        color_cache[idx].color = xrcolor;
    }
    color_cached[idx] = true;
    return color_cache[idx];
}

int main() {

    window.width = 800;
    window.height = 600;

    // Xlib init
    dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;
    int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

    char font_name[50];
    sprintf(font_name, "monospace-%d", font_size);
    xftFont = XftFontOpenName(dpy, DefaultScreen(dpy), font_name);

    XGlyphInfo extents;
    XftTextExtentsUtf8(dpy, xftFont, (FcChar8 *)"M", 1, &extents);
    window.char_width = extents.xOff;
    window.char_height = xftFont->height;

    window.cols = window.width / window.char_width;
    window.rows = window.height / window.char_height;

    XVisualInfo vinfo;
    XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo);
    visual = vinfo.visual;

	colormap = XCreateColormap(dpy, root, visual, AllocNone);

	XSetWindowAttributes attrs;
	attrs.colormap = colormap;
	attrs.background_pixel = BlackPixel(dpy, screen);
	attrs.border_pixel = WhitePixel(dpy, screen);
	attrs.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

	ulong valuemask = CWColormap | CWBackPixel | CWBorderPixel | CWEventMask;

    win = XCreateWindow(dpy, root, 100, 100,
			window.width, window.height, 1, vinfo.depth,
			InputOutput, visual, valuemask, &attrs);
    XMapWindow(dpy, win);

    GC gc = XCreateGC(dpy, win, 0, NULL);
    XFontStruct *font = XLoadQueryFont(dpy, "fixed");
    if (!font) return 1;
    XSetFont(dpy, gc, font->fid);

    xftFont = XftFontOpenName(dpy, screen, font_name);
    draw = XftDrawCreate(dpy, win, visual, colormap);

    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);

    int masterFd;
    pid_t pid = forkpty(&masterFd, NULL, NULL, NULL);
    if (pid == 0) {
        char *shell = getenv("SHELL");
        if (!shell) shell = "/bin/bash";
        execlp(shell, shell, "-l", NULL);
        perror("exec");
        exit(1);
    }

    // libvterm init
    VTerm *vt = vterm_new(window.rows, window.cols);
    vterm_set_utf8(vt, 1);
    vterm_output_set_callback(vt, (void*)write_to_pty, &masterFd);
    VTermScreen *vtermScreen = vterm_obtain_screen(vt);
    vterm_screen_reset(vtermScreen, 1);

    VTermState *vtermState = vterm_obtain_state(vt);
    vterm_state_set_bold_highbright(vtermState, 1);

    VTermColor default_fg, default_bg;
    vterm_state_get_default_colors(vtermState, &default_fg, &default_bg);
    
    vterm_screen_enable_altscreen(vtermScreen, 1);
    
    char buf[1024];

    XftColor xft_bg;
    XRenderColor bg_render_color = {
        .red    = bg_color.red,
        .green  = bg_color.green,
        .blue   = bg_color.blue,
        .alpha  = bg_color.alpha,
    };
    XftColorAllocValue(dpy, visual, colormap, &bg_render_color, &xft_bg);

    XftColor xft_fg;
    XRenderColor fg_render_color = {
        .red    = fg_color.red,
        .green  = fg_color.green,
        .blue   = fg_color.blue,
        .alpha  = fg_color.alpha,
    };
    XftColorAllocValue(dpy, visual, colormap, &fg_render_color, &xft_fg);


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

        bool need_redraw = false;

        // Shell output
        if (FD_ISSET(masterFd, &fds)) {
            int len = read(masterFd, buf, sizeof(buf));
            if (len > 0) {
                vterm_input_write(vt, buf, len);
                vterm_screen_flush_damage(vtermScreen);
            } else if (len == 0) {
                break;
            } else {
                perror("read from shell");
                break;
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
                    int new_cols = new_width / window.char_width;
                    int new_rows = new_height / window.char_height;

                    if (new_cols != window.cols || new_rows != window.rows) {
                        window.width = new_width;
                        window.height = new_height;
                        window.cols = new_cols;
                        window.rows = new_rows;
                        vterm_set_size(vt, window.rows, window.cols);
                        XftDrawChange(draw, win);
                    }
                }
            }
        }

        XftDrawRect(draw, &xft_bg, 0, 0, window.width, window.height);
        VTermScreenCell cell;
        VTermPos pos;

        // Render loop
        for (int row = 0; row < window.rows; row++) {
            for (int col = 0; col < window.cols; col++) {
                pos.row = row;
                pos.col = col;
                if (!vterm_screen_get_cell(vtermScreen, pos, &cell)) continue;
                if (cell.chars[0] == 0) continue;

                vterm_state_convert_color_to_rgb(vtermState, &cell.fg);
                vterm_state_convert_color_to_rgb(vtermState, &cell.bg);

                XftColor xft_fg_cell = xft_color_from_vterm(dpy, visual, colormap, &cell.fg);
                XftColor xft_bg_cell = xft_color_from_vterm(dpy, visual, colormap, &cell.bg);

                if (xft_bg_cell.color.red != bg_render_color.red ||
                    xft_bg_cell.color.green != bg_render_color.green ||
                    xft_bg_cell.color.blue != bg_render_color.blue) {
                    XftDrawRect(draw, &xft_bg_cell,
                        col * window.char_width,
                        row * window.char_height,
                        window.char_width,
                        window.char_height);
                }

                // XSetForeground(dpy, gc, WhitePixel(dpy, screen));

                XftDrawStringUtf8(draw, &xft_fg_cell, xftFont,
                    col * window.char_width,
                    (row + 1) * window.char_height,
                    (FcChar8*)cell.chars,
                    strlen((char*)cell.chars));

                XftColorFree(dpy, visual, colormap, &xft_fg_cell);
                XftColorFree(dpy, visual, colormap, &xft_bg_cell);
            }
        }

        VTermPos cursorPos;
        vterm_state_get_cursorpos(vtermState, &cursorPos);

        XftDrawRect(draw, &xftCursor, cursorPos.col * window.char_width, 
            (cursorPos.row +1) * window.char_height - window.char_height + 8,
            window.char_width, window.char_height - 5);

        XFlush(dpy);
        fflush(stdout);
    }

    close(masterFd);
	XFreeColormap(dpy, colormap);
    XftColorFree(dpy, visual, colormap, &xftCursor);
	vterm_free(vt);
	XCloseDisplay(dpy);
	return 0;
}
