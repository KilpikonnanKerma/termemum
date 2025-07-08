
#ifndef INPUT_H
#define INPUT_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <vterm.h>
#include <unistd.h>

void handle_keypress(XKeyEvent *xKey, VTerm *vt, int masterFd);

#endif