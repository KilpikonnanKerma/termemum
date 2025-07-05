#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <vterm.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pty.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

#define WIDTH 800
#define HEIGHT 600
#define FONT_WIDTH 10
#define FONT_HEIGHT 16
#define COLS (WIDTH / FONT_WIDTH)
#define ROWS (HEIGHT / FONT_HEIGHT)

// Callback to send data from libvterm back to shell
int write_to_pty(const char *s, long unsigned int len, void *user) {
    int *pty_fd = (int *)user;
    return write(*pty_fd, s, len);
}

unsigned short convert_color_component(uint8_t val) {
	return (unsigned short)(val * 257);
}

int main() {
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
    attrs.event_mask = ExposureMask | KeyPressMask;

	unsigned long valuemask = CWColormap | CWBackPixel | CWBorderPixel | CWEventMask;
    
    Window win = XCreateWindow(dpy, root, 100, 100,
			WIDTH, HEIGHT, 1, DefaultDepth(dpy, screen),
			InputOutput, visual, valuemask, &attrs);
    //XSelectInput(dpy, win, ExposureMask | KeyPressMask);
    XMapWindow(dpy, win);

    GC gc = XCreateGC(dpy, win, 0, NULL);
    XFontStruct *font = XLoadQueryFont(dpy, "fixed");
    if (!font) return 1;
    XSetFont(dpy, gc, font->fid);

    XftFont *xftFont = XftFontOpenName(dpy, screen, "monospace-14");
    XftDraw *draw = XftDrawCreate(dpy, win, visual, colormap);

    // PTY + shell
    int master_fd;
    pid_t pid = forkpty(&master_fd, NULL, NULL, NULL);
    if (pid == 0) {
        execlp("bash", "bash", NULL);
        exit(1);
    }

    // libvterm init
    VTerm *vt = vterm_new(ROWS, COLS);
    vterm_set_utf8(vt, 1);
    vterm_output_set_callback(vt, (void*)write_to_pty, &master_fd);
    VTermScreen *vtermScreen = vterm_obtain_screen(vt);
    vterm_screen_reset(vtermScreen, 1);
	
    char buf[1024];

    XftColor xft_fg;
    XRenderColor fg_color = {
        .red = convert_color_component(255),
        .green = convert_color_component(255),
        .blue = convert_color_component(255),
        .alpha = 0xFFFF,
    };

    XftColorAllocValue(dpy, visual, colormap, &fg_color, &xft_fg);

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(master_fd, &fds);
        FD_SET(ConnectionNumber(dpy), &fds);

        int maxfd = (master_fd > ConnectionNumber(dpy)) ? master_fd : ConnectionNumber(dpy);
        if (select(maxfd + 1, &fds, NULL, NULL, NULL) < 0) break;

        // Shell output
        if (FD_ISSET(master_fd, &fds)) {
            int len = read(master_fd, buf, sizeof(buf));
            if (len > 0) {
                vterm_input_write(vt, buf, len);
                vterm_screen_flush_damage(vtermScreen);
            }
        }

        // Keypress input
        if (FD_ISSET(ConnectionNumber(dpy), &fds)) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            if (ev.type == KeyPress) {
                char keybuf[32];
                KeySym keysym;
                int len = XLookupString(&ev.xkey, keybuf, sizeof(keybuf), &keysym, NULL);
                if (len > 0) {
                    for (int i = 0; i < len; i++)
                        vterm_keyboard_unichar(vt, keybuf[i], VTERM_MOD_NONE);
                }
            }
        }

        // Draw screen
        XClearWindow(dpy, win);
        VTermScreenCell cell;
        VTermPos pos;

        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                pos.row = row;
                pos.col = col;
                if (!vterm_screen_get_cell(vtermScreen, pos, &cell)) continue;
                if (cell.chars[0] == 0) continue;

                char ch[5] = {0};
                memcpy(ch, cell.chars, sizeof(cell.chars));

                XSetForeground(dpy, gc, WhitePixel(dpy, screen));
                
                XftDrawStringUtf8(draw, &xft_fg, xftFont, col * FONT_WIDTH, (row + 1) * xftFont->height, (FcChar8*)cell.chars , strlen((char*)cell.chars));
            }
        }
        XFlush(dpy);
    }

	XFreeColormap(dpy, colormap);
    vterm_free(vt);
    XCloseDisplay(dpy);
    return 0;
}
