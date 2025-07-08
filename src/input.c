#include "input.h"


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
        case XK_Left:   vKey = VTERM_KEY_LEFT; break;
        case XK_Right:  vKey = VTERM_KEY_RIGHT; break;
        case XK_Up:     vKey = VTERM_KEY_UP; break;
        case XK_Down:   vKey = VTERM_KEY_DOWN; break;
        // Add more as needed
    }

    if (vKey != VTERM_KEY_NONE) {
        vterm_keyboard_key(vt, vKey, mod);
    } else if (len > 0) {
        // Use libvterm for all input
        vterm_keyboard_unichar(vt, keybuf[0], mod);
    }
}