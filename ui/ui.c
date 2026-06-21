#include "ui.h"

#include <stdio.h>

#include "../include/colors.h"
#include "drawing.h"
#include "../processes/process_list.h"

#define TAB_HEIGHT   36
#define TAB_WIDTH   130
#define TAB_PAD_X    16

static const char *TAB_LABELS[TAB_COUNT] = {
    "Performance",
    "Processes",
    "Network",
};

void ui_init(UiState *ui)
{
    ui->big = CreateFontA(
        18,0,0,0, FW_BOLD,
        0,0,0, DEFAULT_CHARSET,
        0,0, CLEARTYPE_QUALITY,
        0, "Segoe UI");
 
    ui->med = CreateFontA(
        14,0,0,0, FW_NORMAL,
        0,0,0, DEFAULT_CHARSET,
        0,0, CLEARTYPE_QUALITY,
        0, "Segoe UI");
 
    ui->mono = CreateFontA(
        13,0,0,0, FW_NORMAL,
        0,0,0, DEFAULT_CHARSET,
        0,0, CLEARTYPE_QUALITY,
        FIXED_PITCH, "Consolas");

    ui->active_tab = TAB_PERFORMANCE;
}
 
void ui_destroy(UiState *ui)
{
    DeleteObject(ui->big);
    DeleteObject(ui->med);
    DeleteObject(ui->mono);
}

int ui_hit_tab(const UiState *ui, int click_x, int click_y)
{
    (void)ui;
 
    if (click_y < 0 || click_y >= TAB_HEIGHT)
        return -1;
 
    for (int i = 0; i < TAB_COUNT; i++) {
        int x0 = i * TAB_WIDTH;
        int x1 = x0 + TAB_WIDTH;
        if (click_x >= x0 && click_x < x1)
            return i;
    }
 
    return -1;
}

static void draw_tabs(HDC hdc, const UiState *ui, int win_w)
{
    HBRUSH bar_bg = CreateSolidBrush(RGB(15, 15, 25));
    RECT bar = { 0, 0, win_w, TAB_HEIGHT };
    FillRect(hdc, &bar, bar_bg);
    DeleteObject(bar_bg);
 
    HPEN sep_pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 75));
    HPEN old_pen = (HPEN)SelectObject(hdc, sep_pen);
    MoveToEx(hdc, 0, TAB_HEIGHT - 1, NULL);
    LineTo(hdc, win_w, TAB_HEIGHT - 1);
    SelectObject(hdc, old_pen);
    DeleteObject(sep_pen);
 
    for (int i = 0; i < TAB_COUNT; i++) {
        int x0 = i * TAB_WIDTH;
        int active = (i == (int)ui->active_tab);
 
        if (active) {
            HBRUSH hi = CreateSolidBrush(RGB(30, 30, 48));
            RECT tr = { x0, 0, x0 + TAB_WIDTH, TAB_HEIGHT };
            FillRect(hdc, &tr, hi);
            DeleteObject(hi);
 
            HPEN ul = CreatePen(PS_SOLID, 2, CLR_BLUE);
            HPEN op = (HPEN)SelectObject(hdc, ul);
            MoveToEx(hdc, x0 + 6,              TAB_HEIGHT - 2, NULL);
            LineTo  (hdc, x0 + TAB_WIDTH - 6,  TAB_HEIGHT - 2);
            SelectObject(hdc, op);
            DeleteObject(ul);
        }
 
        SelectObject(hdc, ui->med);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, active ? CLR_TEXT : CLR_DIM);
 
        RECT lr = {
            x0 + TAB_PAD_X,
            0,
            x0 + TAB_WIDTH - TAB_PAD_X,
            TAB_HEIGHT
        };
        DrawTextA(hdc, TAB_LABELS[i], -1, &lr,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void paint(
    HWND hwnd,
    const UiState *ui,
    const Stats *stats)
{
    PAINTSTRUCT ps;

    HDC hdc_screen =
        BeginPaint(hwnd, &ps);

    RECT cr;
    GetClientRect(hwnd, &cr);

    HDC hdc =
        CreateCompatibleDC(
            hdc_screen
        );

    HBITMAP bmp =
        CreateCompatibleBitmap(
            hdc_screen,
            cr.right,
            cr.bottom
        );

    SelectObject(hdc, bmp);

    HBRUSH bg =
        CreateSolidBrush(CLR_BG);

    FillRect(hdc, &cr, bg);
    DeleteObject(bg);

    draw_tabs(hdc, ui, cr.right);

    char buf[128];

    int x = 20;
    int y = TAB_HEIGHT + 20;

    int bar_w = cr.right - 160;

    if (ui->active_tab == TAB_PERFORMANCE) {

        draw_label(
            hdc,
            ui->big,
            CLR_BLUE,
            x,
            y,
            300,
            "sysmon"
        );

        y += 30;

        draw_label(
            hdc,
            ui->med,
            CLR_DIM,
            x,
            y,
            80,
            "CPU"
        );

        sprintf(buf,
            "%.1f%%",
            stats->cpu);

        draw_label(
            hdc,
            ui->med,
            CLR_TEXT,
            x + 80,
            y,
            80,
            buf
        );

        draw_bar(
            hdc,
            x + 250,
            y + 3,
            bar_w,
            14,
            stats->cpu
        );

        y += 30;

        draw_label(
            hdc,
            ui->med,
            CLR_DIM,
            x,
            y,
            80,
            "GPU"
        );

        sprintf(buf,
            "%.1f%%",
            stats->gpu);

        draw_label(
            hdc,
            ui->med,
            CLR_TEXT,
            x + 80,
            y,
            80,
            buf
        );

        draw_bar(
            hdc,
            x + 250,
            y + 3,
            bar_w,
            14,
            stats->gpu
        );

        y += 30;

        float mem_pct =
            stats->mem_total_mb > 0
            ? (float)stats->mem_used_mb /
            stats->mem_total_mb * 100.0f
            : 0;

        draw_label(
            hdc,
            ui->med,
            CLR_DIM,
            x,
            y,
            80,
            "Memory"
        );

        sprintf(
            buf,
            "%ld / %ld MB",
            stats->mem_used_mb,
            stats->mem_total_mb
        );

        draw_label(
            hdc,
            ui->med,
            CLR_TEXT,
            x + 80,
            y,
            200,
            buf
        );

        draw_bar(
            hdc,
            x + 250,
            y + 3,
            bar_w,
            14,
            mem_pct
        );

        y += 30;

        float disk_pct =
            stats->disk_total_gb > 0
            ? (float)stats->disk_used_gb /
            stats->disk_total_gb * 100.0f
            : 0;

        draw_label(
            hdc,
            ui->med,
            CLR_DIM,
            x,
            y,
            80,
            "Disk C:"
        );

        sprintf(
            buf,
            "%ld / %ld GB",
            stats->disk_used_gb,
            stats->disk_total_gb
        );

        draw_label(
            hdc,
            ui->med,
            CLR_TEXT,
            x + 80,
            y,
            200,
            buf
        );

        draw_bar(
            hdc,
            x + 250,
            y + 3,
            bar_w,
            14,
            disk_pct
        );

        y += 30;

        draw_label(
            hdc,
            ui->med,
            CLR_DIM,
            x,
            y,
            80,
            "Network"
        );

        sprintf(
            buf,
            "RX %lu KB/s TX %lu KB/s",
            stats->net_rx_kb,
            stats->net_tx_kb
        );

        draw_label(
            hdc,
            ui->med,
            CLR_TEXT,
            x + 80,
            y,
            400,
            buf
        );

        y += 40;

        Proc procs[15];

        int count =
            get_top_processes(
                procs,
                15
            );

        for (int i = 0;
            i < count &&
            y + 20 < cr.bottom - 10;
            i++)
        {
            sprintf(
                buf,
                "%d",
                procs[i].pid
            );

            draw_label(
                hdc,
                ui->mono,
                CLR_DIM,
                x,
                y,
                60,
                buf
            );

            draw_label(
                hdc,
                ui->mono,
                CLR_TEXT,
                x + 70,
                y,
                200,
                procs[i].name
            );

            sprintf(
                buf,
                "%.1f MB",
                procs[i].mem_kb / 1024.0
            );

            draw_label(
                hdc,
                ui->mono,
                CLR_DIM,
                x + 280,
                y,
                120,
                buf
            );

            y += 20;
        }
    }

    BitBlt(
        hdc_screen,
        0,
        0,
        cr.right,
        cr.bottom,
        hdc,
        0,
        0,
        SRCCOPY
    );

    DeleteObject(bmp);
    DeleteDC(hdc);

    EndPaint(hwnd, &ps);
}