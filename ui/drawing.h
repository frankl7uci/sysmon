#ifndef DRAWING_H
#define DRAWING_H

#include <windows.h>

COLORREF bar_color(float pct);

void draw_label(
    HDC hdc,
    HFONT font,
    COLORREF color,
    int x,
    int y,
    int w,
    const char *text
);

void draw_bar(
    HDC hdc,
    int x,
    int y,
    int w,
    int h,
    float pct
);

#endif