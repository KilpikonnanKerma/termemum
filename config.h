#ifndef _CONFIG_H
#define _CONFIG_H

#include "win.h"
/*

        COPYRIGHT (C) 2025 Nico Rajala

        THIS IS THE CONFIG FILE FOR TERMEMUM

        After editing this file, you need to recompile the source code with:
            ` make install `


        Edit at your own risk!

*/

// Default font size
static int font_size = 16;

// Background color (Use hex color codes eg. #FFFF for white)
static Color bg_color = {
    .red = 0x0000,
    .green = 0x0000,
    .blue = 0x0000,
    .alpha = 0xFFFF,
};

// Default foreground color (Use hex color codes eg. #FFFF for white)
static Color fg_color = {
    .red = 0xFFFF,
    .green = 0xFFFF,
    .blue = 0xFFFF,
    .alpha = 0xFFFF,
};


/*

    Keybinds can't be customized yet. But here are some pre-defined keybinds:

        Ctrl + PageUp       Zoom in
        Ctrl + PageDown     Zoom out

*/


#endif