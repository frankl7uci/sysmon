#include "drawing.h"
#include "../include/colors.h"
 
COLORREF bar_color(float pct)
{
    if (pct < 60.0f)  return CLR_GREEN;
    if (pct < 85.0f)  return RGB(250, 200, 80);
    return CLR_RED;
}
 
void draw_label(
    HDC hdc,
    HFONT font,
    COLORREF color,
    int x, int y, int w,
    const char *text)
{
    SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    RECT r = { x, y, x + w, y + 20 };
    DrawTextA(hdc, text, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}
 
void draw_bar(
    HDC hdc,
    int x, int y, int w, int h,
    float pct)
{
    HBRUSH bg = CreateSolidBrush(RGB(45, 45, 65));
    RECT track = { x, y, x + w, y + h };
    FillRect(hdc, &track, bg);
    DeleteObject(bg);
 
    int filled = (int)(pct / 100.0f * w);
    if (filled > 0) {
        HBRUSH fg = CreateSolidBrush(bar_color(pct));
        RECT fill = { x, y, x + filled, y + h };
        FillRect(hdc, &fill, fg);
        DeleteObject(fg);
    }
}

void draw_graph(
    HDC hdc,
    int x, int y, int w, int h,
    const History *hist,
    float y_max,
    COLORREF line_color)
{
    HBRUSH bg = CreateSolidBrush(RGB(18, 18, 30));
    RECT area = { x, y, x + w, y + h };
    FillRect(hdc, &area, bg);
    DeleteObject(bg);

    HPEN border = CreatePen(PS_SOLID, 1, RGB(50, 50, 75));
    HPEN old_pen = (HPEN)SelectObject(hdc, border);
    MoveToEx(hdc, x,         y,         NULL);
    LineTo  (hdc, x + w - 1, y);
    LineTo  (hdc, x + w - 1, y + h - 1);
    LineTo  (hdc, x,         y + h - 1);
    LineTo  (hdc, x,         y);
    SelectObject(hdc, old_pen);
    DeleteObject(border);
 
    if (hist->count < 2)
        return;

    HPEN grid = CreatePen(PS_SOLID, 1, RGB(35, 35, 55));
    SelectObject(hdc, grid);
    for (int g = 1; g <= 3; g++) {
        int gy = y + h - (int)(g / 4.0f * h);
        MoveToEx(hdc, x + 1, gy, NULL);
        LineTo  (hdc, x + w - 1, gy);
    }
    SelectObject(hdc, old_pen);
    DeleteObject(grid);
 
    float ceiling = (y_max > 0.0f) ? y_max : history_max(hist);
    if (ceiling < 1.0f) ceiling = 1.0f;

    int n = hist->count;
    POINT *pts = (POINT *)malloc((n + 2) * sizeof(POINT));
    if (!pts) return;
 
    int inner_w = w - 2;
    int inner_h = h - 2;
 
    for (int i = 0; i < n; i++) {
        float val = history_get(hist, n - 1 - i);
        int px = x + 1 + (int)((float)i / (HISTORY_LEN - 1) * inner_w);
        int py = y + 1 + inner_h - (int)(val / ceiling * inner_h);
        if (py < y + 1)       py = y + 1;
        if (py > y + h - 2)   py = y + h - 2;
        pts[i].x = px;
        pts[i].y = py;
    }

    pts[n].x     = pts[n - 1].x;
    pts[n].y     = y + h - 1;
    pts[n + 1].x = pts[0].x;
    pts[n + 1].y = y + h - 1;

    HBRUSH fill_brush = CreateSolidBrush(
        RGB(
            GetRValue(line_color) / 6,
            GetGValue(line_color) / 6,
            GetBValue(line_color) / 6
        )
    );
    HBRUSH old_brush = (HBRUSH)SelectObject(hdc, fill_brush);
    HPEN fill_pen    = CreatePen(PS_NULL, 0, 0);
    SelectObject(hdc, fill_pen);
    Polygon(hdc, pts, n + 2);
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(fill_brush);
    DeleteObject(fill_pen);

    HPEN line_pen = CreatePen(PS_SOLID, 2, line_color);
    SelectObject(hdc, line_pen);
    MoveToEx(hdc, pts[0].x, pts[0].y, NULL);
    for (int i = 1; i < n; i++)
        LineTo(hdc, pts[i].x, pts[i].y);
    SelectObject(hdc, old_pen);
    DeleteObject(line_pen);
 
    free(pts);
}