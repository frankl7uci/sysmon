#include "ui.h"

#include <stdio.h>

#include "../include/colors.h"
#include "drawing.h"
#include "../processes/process_list.h"

void paint(
    HWND hwnd,
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

    HFONT big =
        CreateFontA(
            18,0,0,0,
            FW_BOLD,
            0,0,0,
            DEFAULT_CHARSET,
            0,0,
            CLEARTYPE_QUALITY,
            0,
            "Segoe UI"
        );

    HFONT med =
        CreateFontA(
            14,0,0,0,
            FW_NORMAL,
            0,0,0,
            DEFAULT_CHARSET,
            0,0,
            CLEARTYPE_QUALITY,
            0,
            "Segoe UI"
        );

    HFONT mono =
        CreateFontA(
            13,0,0,0,
            FW_NORMAL,
            0,0,0,
            DEFAULT_CHARSET,
            0,0,
            CLEARTYPE_QUALITY,
            FIXED_PITCH,
            "Consolas"
        );

    char buf[128];

    int x = 20;
    int y = 20;

    int bar_w =
        cr.right - 160;

    draw_label(
        hdc,
        big,
        CLR_BLUE,
        x,
        y,
        300,
        "sysmon"
    );

    y += 30;

    draw_label(
        hdc,
        med,
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
        med,
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
        med,
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
        med,
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
        med,
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
        med,
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
        med,
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
        med,
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
        med,
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
        med,
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
            mono,
            CLR_DIM,
            x,
            y,
            60,
            buf
        );

        draw_label(
            hdc,
            mono,
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
            mono,
            CLR_DIM,
            x + 280,
            y,
            120,
            buf
        );

        y += 20;
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

    DeleteObject(big);
    DeleteObject(med);
    DeleteObject(mono);

    DeleteObject(bmp);
    DeleteDC(hdc);

    EndPaint(hwnd, &ps);
}