
#ifndef _INPUT_H
#define _INPUT_H

#include <X11/keysym.h>
#include <vterm.h>
#include <unistd.h>
#include "win.h"

void handle_keypress(XKeyEvent *xKey, VTerm *vt, int masterFd);
void handle_zoom(VTerm *vt, int zoom);

#endif