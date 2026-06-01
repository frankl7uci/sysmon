#include "drawing.h"
#include "../include/colors.h"

COLORREF bar_color(float pct)
{
    if (pct < 60.0f)
        return CLR_GREEN;

    if (pct < 85.0f)
        return RGB(250, 200, 80);

    return CLR_RED;
}

void draw_label(
    HDC hdc,
    HFONT font,
    COLORREF color,
    int x,
    int y,
    int w,
    const char *text)
{
    SelectObject(hdc, font);

    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    RECT r = { x, y, x + w, y + 20 };

    DrawTextA(
        hdc,
        text,
        -1,
        &r,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE
    );
}

void draw_bar(
    HDC hdc,
    int x,
    int y,
    int w,
    int h,
    float pct)
{
    HBRUSH bg = CreateSolidBrush(
        RGB(45, 45, 65)
    );

    RECT track = {
        x,
        y,
        x + w,
        y + h
    };

    FillRect(hdc, &track, bg);
    DeleteObject(bg);

    int filled =
        (int)(pct / 100.0f * w);

    if (filled > 0) {

        HBRUSH fg =
            CreateSolidBrush(
                bar_color(pct)
            );

        RECT fill = {
            x,
            y,
            x + filled,
            y + h
        };

        FillRect(hdc, &fill, fg);

        DeleteObject(fg);
    }
}