#include "input.h"
#include "config.h"


void handle_keypress(XKeyEvent *xKey, VTerm *vt, int masterFd) {
    char keybuf[32];
    KeySym keysym;
    int len = XLookupString(xKey, keybuf, sizeof(keybuf), &keysym, NULL);

    VTermModifier mod = VTERM_MOD_NONE;
    if(xKey->state & ShiftMask) mod |= VTERM_MOD_SHIFT;
    if(xKey->state & ControlMask) mod |= VTERM_MOD_CTRL;
    if(xKey->state & Mod1Mask) mod |= VTERM_MOD_ALT;

    VTermKey vKey = VTERM_KEY_NONE;
    switch(keysym) {
        case XK_Left:       vKey = VTERM_KEY_LEFT; break;
        case XK_Right:      vKey = VTERM_KEY_RIGHT; break;
        case XK_Up:         vKey = VTERM_KEY_UP; break;
        case XK_Down:       vKey = VTERM_KEY_DOWN; break;
        case XK_Page_Up:    vKey = VTERM_KEY_PAGEUP; break;
        case XK_Page_Down:  vKey = VTERM_KEY_PAGEDOWN; break;
    }

    if (mod == VTERM_MOD_CTRL && (vKey == VTERM_KEY_PAGEUP || vKey == VTERM_KEY_PAGEDOWN)) {
        handle_zoom(vt, vKey == VTERM_KEY_PAGEUP? 1 : -1);
    } else if (vKey != VTERM_KEY_NONE) {
        vterm_keyboard_key(vt, vKey, mod);
    } else if (len > 0) {
        // Use libvterm for all input
        vterm_keyboard_unichar(vt, keybuf[0], mod);
    }
}

void handle_zoom(VTerm *vt, int zoom) {
    font_size += zoom;

    if(font_size < 8) font_size = 8;
    if(font_size > 24) font_size = 32;

    if(xftFont) XftFontClose(dpy, xftFont);

    char font_name[50];
    sprintf(font_name, "monospace-%d", font_size);
    xftFont = XftFontOpenName(dpy, DefaultScreen(dpy), font_name);

    XGlyphInfo extents;
    XftTextExtentsUtf8(dpy, xftFont, (FcChar8 *)"M", 1, &extents);
    window.char_width = extents.xOff;
    window.char_height = xftFont->height;

    window.cols = window.width / window.char_width;
    window.rows = window.height / window.char_height;

    vterm_set_size(vt, window.rows, window.cols);

    XftDrawChange(draw, win);
}