#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/colors.h"
#include "drawing.h"
#include "../processes/process_list.h"

#define TAB_HEIGHT   36
#define TAB_WIDTH   130
#define TAB_PAD_X    16

#define CARD_W         220
#define CARD_H          72
#define CARD_PAD_X      12
#define CARD_PAD_Y      10
#define CARD_GAP        10
#define CARD_X_ORIGIN   10
#define CARD_Y_ORIGIN   (TAB_HEIGHT + 10)

#define COL_PID_X        10
#define COL_NAME_X       80
#define COL_MEM_X       320
#define COL_HDR_Y       (TAB_HEIGHT + 8)
#define COL_HDR_H        22
#define ROW_H            20

static const char *TAB_LABELS[TAB_COUNT] = {
    "Performance",
    "Processes",
    "Network",
};

static const char *METRIC_LABELS[METRIC_COUNT] = {
    "CPU", "GPU", "Memory", "Disk", "Network"
};
static const COLORREF METRIC_COLORS[METRIC_COUNT] = {
    /* CPU */     RGB( 99, 179, 237),
    /* GPU */     RGB(154, 117, 234),
    /* Memory */  RGB( 72, 199, 142),
    /* Disk */    RGB(250, 200,  80),
    /* Network */ RGB(252, 100,  90),
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

    ui->active_tab      = TAB_PERFORMANCE;
    ui->selected_metric = METRIC_CPU;
    ui->proc_sort       = SORT_MEMORY;

    memset(ui->hist, 0, sizeof(ui->hist));
}
 
void ui_destroy(UiState *ui)
{
    DeleteObject(ui->big);
    DeleteObject(ui->med);
    DeleteObject(ui->mono);
}

void ui_push_history(UiState *ui, const Stats *s)
{
    history_push(&ui->hist[METRIC_CPU],    s->cpu);
    history_push(&ui->hist[METRIC_GPU],    s->gpu);
 
    float mem_pct = s->mem_total_mb > 0
        ? (float)s->mem_used_mb / s->mem_total_mb * 100.0f : 0;
    history_push(&ui->hist[METRIC_MEMORY], mem_pct);
 
    float disk_pct = s->disk_total_gb > 0
        ? (float)s->disk_used_gb / s->disk_total_gb * 100.0f : 0;
    history_push(&ui->hist[METRIC_DISK],   disk_pct);

    float net = (float)(s->net_rx_kb + s->net_tx_kb);
    history_push(&ui->hist[METRIC_NETWORK], net);
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

int ui_hit_metric_card(int click_x, int click_y, int win_h)
{
    (void)win_h;
    for (int i = 0; i < METRIC_COUNT; i++) {
        int cx = CARD_X_ORIGIN;
        int cy = CARD_Y_ORIGIN + i * (CARD_H + CARD_GAP);
        if (click_x >= cx && click_x < cx + CARD_W &&
            click_y >= cy && click_y < cy + CARD_H)
            return i;
    }
    return -1;
}

int ui_hit_proc_col(int click_x, int click_y)
{
    if (click_y < COL_HDR_Y || click_y >= COL_HDR_Y + COL_HDR_H)
        return -1;
    if (click_x >= COL_PID_X  && click_x < COL_NAME_X) return SORT_PID;
    if (click_x >= COL_NAME_X && click_x < COL_MEM_X)  return SORT_NAME;
    if (click_x >= COL_MEM_X  && click_x < COL_MEM_X + 160) return SORT_MEMORY;
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

static void draw_metric_card(
    HDC hdc, const UiState *ui,
    int idx, const char *value_str, int selected)
{
    int cx = CARD_X_ORIGIN;
    int cy = CARD_Y_ORIGIN + idx * (CARD_H + CARD_GAP);
    COLORREF col = METRIC_COLORS[idx];

    HBRUSH card_bg = CreateSolidBrush(
        selected ? RGB(35, 35, 55) : RGB(25, 25, 38));
    RECT cr = { cx, cy, cx + CARD_W, cy + CARD_H };
    FillRect(hdc, &cr, card_bg);
    DeleteObject(card_bg);

    HBRUSH accent = CreateSolidBrush(col);
    RECT ab = { cx, cy, cx + 3, cy + CARD_H };
    FillRect(hdc, &ab, accent);
    DeleteObject(accent);

    SelectObject(hdc, ui->med);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, CLR_DIM);
    RECT lr = { cx + 10, cy + 6, cx + CARD_W - 6, cy + 26 };
    DrawTextA(hdc, METRIC_LABELS[idx], -1, &lr, DT_LEFT | DT_SINGLELINE);

    SelectObject(hdc, ui->big);
    SetTextColor(hdc, col);
    RECT vr = { cx + 10, cy + 28, cx + CARD_W - 6, cy + CARD_H - 6 };
    DrawTextA(hdc, value_str, -1, &vr, DT_LEFT | DT_SINGLELINE);

    if (selected) {
        HPEN pen = CreatePen(PS_SOLID, 1, col);
        HPEN op  = (HPEN)SelectObject(hdc, pen);
        HBRUSH nb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, cx, cy, cx + CARD_W, cy + CARD_H);
        SelectObject(hdc, op);
        SelectObject(hdc, nb);
        DeleteObject(pen);
    }
}

static void draw_metric_detail(
    HDC hdc, const UiState *ui,
    int x, int y, int w, int h,
    int metric_idx,
    const Stats *stats)
{
    COLORREF col   = METRIC_COLORS[metric_idx];
    const History *hist = &ui->hist[metric_idx];

    SelectObject(hdc, ui->big);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, col);
    RECT tr = { x, y, x + w, y + 24 };
    DrawTextA(hdc, METRIC_LABELS[metric_idx], -1, &tr,
              DT_LEFT | DT_SINGLELINE);
    y += 28;

    int graph_h = h - 28 - 90;
    if (graph_h < 40) graph_h = 40;
 
    float y_max = (metric_idx == METRIC_NETWORK) ? 0.0f : 100.0f;
    draw_graph(hdc, x, y, w, graph_h, hist, y_max, col);
    y += graph_h + 12;

    char buf[128];
    float cur = history_get(hist, 0);
    float mn  = history_min(hist);
    float mx  = history_max(hist);
 
    SelectObject(hdc, ui->med);
    SetTextColor(hdc, CLR_DIM);
 
    if (metric_idx == METRIC_MEMORY) {
        sprintf(buf, "Used:    %ld / %ld MB",
                stats->mem_used_mb, stats->mem_total_mb);
        RECT r = { x, y, x + w, y + 20 };
        DrawTextA(hdc, buf, -1, &r, DT_LEFT | DT_SINGLELINE);
        y += 22;
        sprintf(buf, "Current: %.1f%%   Min: %.1f%%   Max: %.1f%%",
                cur, mn, mx);
        r.top = y; r.bottom = y + 20;
        DrawTextA(hdc, buf, -1, &r, DT_LEFT | DT_SINGLELINE);
 
    } else if (metric_idx == METRIC_DISK) {
        sprintf(buf, "Used:    %ld / %ld GB",
                stats->disk_used_gb, stats->disk_total_gb);
        RECT r = { x, y, x + w, y + 20 };
        DrawTextA(hdc, buf, -1, &r, DT_LEFT | DT_SINGLELINE);
        y += 22;
        sprintf(buf, "Current: %.1f%%   Min: %.1f%%   Max: %.1f%%",
                cur, mn, mx);
        r.top = y; r.bottom = y + 20;
        DrawTextA(hdc, buf, -1, &r, DT_LEFT | DT_SINGLELINE);
 
    } else if (metric_idx == METRIC_NETWORK) {
        sprintf(buf, "RX: %lu KB/s    TX: %lu KB/s",
                stats->net_rx_kb, stats->net_tx_kb);
        RECT r = { x, y, x + w, y + 20 };
        DrawTextA(hdc, buf, -1, &r, DT_LEFT | DT_SINGLELINE);
        y += 22;
        sprintf(buf, "Total:   %.0f KB/s   Min: %.0f   Max: %.0f",
                cur, mn, mx);
        r.top = y; r.bottom = y + 20;
        DrawTextA(hdc, buf, -1, &r, DT_LEFT | DT_SINGLELINE);
 
    } else {
        sprintf(buf, "Current: %.1f%%   Min: %.1f%%   Max: %.1f%%",
                cur, mn, mx);
        RECT r = { x, y, x + w, y + 20 };
        DrawTextA(hdc, buf, -1, &r, DT_LEFT | DT_SINGLELINE);
    }
}

static void paint_performance(
    HDC hdc, const UiState *ui,
    int win_w, int win_h,
    const Stats *stats)
{
    char buf[128];

    char card_vals[METRIC_COUNT][32];
    sprintf(card_vals[METRIC_CPU],     "%.1f%%",           stats->cpu);
    sprintf(card_vals[METRIC_GPU],     "%.1f%%",           stats->gpu);
    sprintf(card_vals[METRIC_MEMORY],  "%ld / %ld MB",
            stats->mem_used_mb, stats->mem_total_mb);
    sprintf(card_vals[METRIC_DISK],    "%ld / %ld GB",
            stats->disk_used_gb, stats->disk_total_gb);
    sprintf(card_vals[METRIC_NETWORK], "RX %lu  TX %lu KB/s",
            stats->net_rx_kb, stats->net_tx_kb);
 
    for (int i = 0; i < METRIC_COUNT; i++)
        draw_metric_card(hdc, ui, i, card_vals[i],
                         i == (int)ui->selected_metric);

    int rx = CARD_X_ORIGIN + CARD_W + 20;
    int ry = TAB_HEIGHT + 10;
    int rw = win_w - rx - 16;
    int rh = win_h - ry - 16;
    if (rw > 0 && rh > 0)
        draw_metric_detail(hdc, ui, rx, ry, rw, rh,
                           ui->selected_metric, stats);

 
    (void)buf;
}

static int cmp_mem_desc(const void *a, const void *b)
{
    const Proc *pa = (const Proc *)a;
    const Proc *pb = (const Proc *)b;
    if (pb->mem_kb > pa->mem_kb) return  1;
    if (pb->mem_kb < pa->mem_kb) return -1;
    return 0;
}
static int cmp_name_asc(const void *a, const void *b)
{
    return strcmp(((const Proc *)a)->name, ((const Proc *)b)->name);
}
static int cmp_pid_asc(const void *a, const void *b)
{
    return ((const Proc *)a)->pid - ((const Proc *)b)->pid;
}

 
static void paint_processes(
    HDC hdc, const UiState *ui,
    int win_w, int win_h)
{
    Proc procs[256];
    int count = get_top_processes(procs, 256);

    switch (ui->proc_sort) {
        case SORT_MEMORY: qsort(procs, count, sizeof(Proc), cmp_mem_desc);  break;
        case SORT_NAME:   qsort(procs, count, sizeof(Proc), cmp_name_asc);  break;
        case SORT_PID:    qsort(procs, count, sizeof(Proc), cmp_pid_asc);   break;
        default: break;
    }

    HBRUSH hdr_bg = CreateSolidBrush(RGB(22, 22, 35));
    RECT hdr_rect = { 0, COL_HDR_Y, win_w, COL_HDR_Y + COL_HDR_H };
    FillRect(hdc, &hdr_rect, hdr_bg);
    DeleteObject(hdr_bg);

    HPEN ul = CreatePen(PS_SOLID, 1, RGB(50, 50, 75));
    HPEN op = (HPEN)SelectObject(hdc, ul);
    MoveToEx(hdc, 0,     COL_HDR_Y + COL_HDR_H - 1, NULL);
    LineTo  (hdc, win_w, COL_HDR_Y + COL_HDR_H - 1);
    SelectObject(hdc, op);
    DeleteObject(ul);

    struct { int x; int w; const char *label; ProcSort sort; } cols[] = {
        { COL_PID_X,  COL_NAME_X - COL_PID_X,  "PID",    SORT_PID    },
        { COL_NAME_X, COL_MEM_X  - COL_NAME_X,  "Name",   SORT_NAME   },
        { COL_MEM_X,  160,                       "Memory", SORT_MEMORY },
    };
 
    for (int c = 0; c < 3; c++) {
        int active = (cols[c].sort == ui->proc_sort);
        SelectObject(hdc, ui->med);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, active ? CLR_BLUE : CLR_DIM);
        RECT lr = {
            cols[c].x, COL_HDR_Y,
            cols[c].x + cols[c].w, COL_HDR_Y + COL_HDR_H
        };
        DrawTextA(hdc, cols[c].label, -1, &lr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        if (active) {
            SelectObject(hdc, ui->mono);
            RECT ar = {
                cols[c].x + 50, COL_HDR_Y,
                cols[c].x + 70, COL_HDR_Y + COL_HDR_H
            };
            DrawTextA(hdc, (cols[c].sort == SORT_MEMORY) ? "v" : "^",
                      -1, &ar, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
    }

    int y = COL_HDR_Y + COL_HDR_H + 4;
    char buf[128];
 
    for (int i = 0; i < count && y + ROW_H < win_h - 4; i++) {
        if (i % 2 == 0) {
            HBRUSH row_bg = CreateSolidBrush(RGB(22, 22, 34));
            RECT rr = { 0, y, win_w, y + ROW_H };
            FillRect(hdc, &rr, row_bg);
            DeleteObject(row_bg);
        }
 
        sprintf(buf, "%d", procs[i].pid);
        draw_label(hdc, ui->mono, CLR_DIM,  COL_PID_X,  y, 60,  buf);
        draw_label(hdc, ui->mono, CLR_TEXT, COL_NAME_X, y, 230, procs[i].name);
        sprintf(buf, "%.1f MB", procs[i].mem_kb / 1024.0);
        draw_label(hdc, ui->mono, CLR_DIM,  COL_MEM_X,  y, 120, buf);
 
        y += ROW_H;
    }
}

static void paint_network(
    HDC hdc, const UiState *ui,
    int win_w, int win_h,
    const Stats *stats)
{
    int y   = TAB_HEIGHT + 14;
    int pad = 20;
    int gw  = (win_w - pad * 3) / 2;
    int gh  = win_h - y - 90;
    if (gh < 60) gh = 60;

    History rx_hist = {0};

    const History *combined = &ui->hist[METRIC_NETWORK];

    SelectObject(hdc, ui->big);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, CLR_GREEN);
    RECT rl = { pad, y, pad + gw, y + 24 };
    DrawTextA(hdc, "Receive (RX)", -1, &rl, DT_LEFT | DT_SINGLELINE);
 
    draw_graph(hdc, pad, y + 28, gw, gh, combined, 0.0f, CLR_GREEN);

    int tx_x = pad * 2 + gw;
    SelectObject(hdc, ui->big);
    SetTextColor(hdc, CLR_RED);
    RECT tl = { tx_x, y, tx_x + gw, y + 24 };
    DrawTextA(hdc, "Transmit (TX)", -1, &tl, DT_LEFT | DT_SINGLELINE);
 
    draw_graph(hdc, tx_x, y + 28, gw, gh, combined, 0.0f, CLR_RED);

    int sy = y + 28 + gh + 12;
    char buf[128];
 
    sprintf(buf, "RX: %lu KB/s", stats->net_rx_kb);
    draw_label(hdc, ui->med, CLR_GREEN, pad, sy, gw, buf);
 
    sprintf(buf, "TX: %lu KB/s", stats->net_tx_kb);
    draw_label(hdc, ui->med, CLR_RED, tx_x, sy, gw, buf);
 
    sy += 22;
    float mn = history_min(combined);
    float mx = history_max(combined);
    sprintf(buf, "Min: %.0f KB/s   Max: %.0f KB/s", mn, mx);
    draw_label(hdc, ui->med, CLR_DIM, pad, sy, win_w - pad * 2, buf);
}

void paint(
    HWND hwnd,
    UiState *ui,
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

    switch (ui->active_tab) {
        case TAB_PERFORMANCE:
            paint_performance(hdc, ui, cr.right, cr.bottom, stats);
            break;
        case TAB_PROCESSES:
            paint_processes(hdc, ui, cr.right, cr.bottom);
            break;
        case TAB_NETWORK:
            paint_network(hdc, ui, cr.right, cr.bottom, stats);
            break;
        default:
            break;
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