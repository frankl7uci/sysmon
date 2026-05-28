/*
 * sysmon.c: basic windows system monitor prototype
 * To build, run: gcc sysmon.c -o sysmon.exe -mwindows -lgdi32 -luser32 -liphlpapi -lpsapi
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string.h>

#include <pdh.h>
#include <pdhmsg.h>

#pragma comment(lib, "pdh.lib")

//Data

typedef struct {
    float cpu;
    float gpu;
    long  mem_used_mb;
    long  mem_total_mb;
    long  disk_used_gb;
    long  disk_total_gb;
    unsigned long net_rx_kb;
    unsigned long net_tx_kb;
} Stats;

static Stats g_stats;

//Collectors

static float get_cpu(void)
{
    static ULONGLONG prev_idle = 0, prev_kernel = 0, prev_user = 0;

    FILETIME fi, fk, fu;
    if (!GetSystemTimes(&fi, &fk, &fu)) return 0.0f;

    ULONGLONG idle   = ((ULONGLONG)fi.dwHighDateTime << 32) | fi.dwLowDateTime;
    ULONGLONG kernel = ((ULONGLONG)fk.dwHighDateTime << 32) | fk.dwLowDateTime;
    ULONGLONG user   = ((ULONGLONG)fu.dwHighDateTime << 32) | fu.dwLowDateTime;

    ULONGLONG d_idle  = idle   - prev_idle;
    ULONGLONG d_total = (kernel - prev_kernel) + (user - prev_user);

    float usage = d_total > 0 ? (float)(d_total - d_idle) / d_total * 100.0f : 0.0f;

    prev_idle = idle; prev_kernel = kernel; prev_user = user;
    return usage;
}

static float get_gpu(void)
{
    static PDH_HQUERY query = NULL;
    static PDH_HCOUNTER counter;
    static int initialized = 0;

    if (!initialized) {
        if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS)
            return 0.0f;

        //Total GPU utilization (seems to only work with Windows 10 and newer)

        if (PdhAddEnglishCounterA(
                query,
                "\\GPU Engine(*)\\Utilization Percentage",
                0,
                &counter) != ERROR_SUCCESS)
        {
            return 0.0f;
        }

        PdhCollectQueryData(query);

        initialized = 1;
        return 0.0f;
    }

    if (PdhCollectQueryData(query) != ERROR_SUCCESS)
        return 0.0f;

    DWORD buf_size = 0;
    DWORD item_count = 0;

    PDH_STATUS status = PdhGetFormattedCounterArrayA(
        counter,
        PDH_FMT_DOUBLE,
        &buf_size,
        &item_count,
        NULL
    );

    if (status != PDH_MORE_DATA)
        return 0.0f;

    PDH_FMT_COUNTERVALUE_ITEM_A *items =
        (PDH_FMT_COUNTERVALUE_ITEM_A*)malloc(buf_size);

    if (!items)
        return 0.0f;

    status = PdhGetFormattedCounterArrayA(
        counter,
        PDH_FMT_DOUBLE,
        &buf_size,
        &item_count,
        items
    );

    if (status != ERROR_SUCCESS) {
        free(items);
        return 0.0f;
    }

    double total = 0.0;

    for (DWORD i = 0; i < item_count; i++) {

        //Sum everything

        if (items[i].FmtValue.CStatus == ERROR_SUCCESS)
            total += items[i].FmtValue.doubleValue;
    }

    free(items);

    // Limit number to 100 for display even when usage can exceed 100%

    if (total > 100.0)
        total = 100.0;

    return (float)total;
}

static void get_memory(long *used, long *total)
{
    MEMORYSTATUSEX m;
    m.dwLength = sizeof(m);
    GlobalMemoryStatusEx(&m);
    *total = (long)(m.ullTotalPhys  / (1024 * 1024));
    *used  = (long)((m.ullTotalPhys - m.ullAvailPhys) / (1024 * 1024));
}

static void get_disk(long *used, long *total)
{
    ULARGE_INTEGER free_b, total_b, dummy;
    GetDiskFreeSpaceExA("C:\\", &free_b, &total_b, &dummy);
    *total = (long)(total_b.QuadPart / (1024*1024*1024));
    *used  = (long)((total_b.QuadPart - free_b.QuadPart) / (1024*1024*1024));
}

static void get_network(unsigned long *rx_kb, unsigned long *tx_kb)
{
    static unsigned long prev_rx = 0, prev_tx = 0;

    DWORD size = 0;
    GetIfTable(NULL, &size, FALSE);
    MIB_IFTABLE *t = (MIB_IFTABLE*)malloc(size);
    if (!t) return;
    GetIfTable(t, &size, FALSE);

    unsigned long rx = 0, tx = 0;
    for (DWORD i = 0; i < t->dwNumEntries; i++) {
        if (t->table[i].dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        rx += t->table[i].dwInOctets;
        tx += t->table[i].dwOutOctets;
    }
    free(t);

    *rx_kb = (rx >= prev_rx) ? (rx - prev_rx) / 1024 : 0;
    *tx_kb = (tx >= prev_tx) ? (tx - prev_tx) / 1024 : 0;
    prev_rx = rx; prev_tx = tx;
}

static void update_stats(void)
{
    g_stats.cpu = get_cpu();
    g_stats.gpu = get_gpu();
    get_memory(&g_stats.mem_used_mb,  &g_stats.mem_total_mb);
    get_disk  (&g_stats.disk_used_gb, &g_stats.disk_total_gb);
    get_network(&g_stats.net_rx_kb,   &g_stats.net_tx_kb);
}

//Drawing

#define CLR_BG    RGB(20,  20,  30)
#define CLR_CARD  RGB(30,  30,  45)
#define CLR_TEXT  RGB(220, 220, 235)
#define CLR_DIM   RGB(120, 120, 150)
#define CLR_BLUE  RGB(99,  179, 237)
#define CLR_GREEN RGB(72,  199, 142)
#define CLR_RED   RGB(252, 100,  90)

static COLORREF bar_color(float pct)
{
    if (pct < 60.0f) return CLR_GREEN;
    if (pct < 85.0f) return RGB(250, 200, 80);
    if (pct < 85.0f) RGB(250, 200, 80);
    return CLR_RED;
}

static void draw_label(HDC hdc, HFONT font, COLORREF color,
                       int x, int y, int w, const char *text)
{
    SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    RECT r = { x, y, x + w, y + 20 };
    DrawTextA(hdc, text, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

static void draw_bar(HDC hdc, int x, int y, int w, int h, float pct)
{
    //track
    HBRUSH bg = CreateSolidBrush(RGB(45, 45, 65));
    RECT track = { x, y, x + w, y + h };
    FillRect(hdc, &track, bg);
    DeleteObject(bg);

    //fill
    int filled = (int)(pct / 100.0f * w);
    if (filled > 0) {
        HBRUSH fg = CreateSolidBrush(bar_color(pct));
        RECT fill = { x, y, x + filled, y + h };
        FillRect(hdc, &fill, fg);
        DeleteObject(fg);
    }
}

static void paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc_screen = BeginPaint(hwnd, &ps);

    //Double buffer
    RECT cr; GetClientRect(hwnd, &cr);
    HDC     hdc  = CreateCompatibleDC(hdc_screen);
    HBITMAP bmp  = CreateCompatibleBitmap(hdc_screen, cr.right, cr.bottom);
    SelectObject(hdc, bmp);

    //Background
    HBRUSH bgbr = CreateSolidBrush(CLR_BG);
    FillRect(hdc, &cr, bgbr);
    DeleteObject(bgbr);

    //Font
    HFONT big  = CreateFontA(18, 0, 0, 0, FW_BOLD,   0,0,0, DEFAULT_CHARSET,0,0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    HFONT med  = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0,0,0, DEFAULT_CHARSET,0,0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    HFONT mono = CreateFontA(13, 0, 0, 0, FW_NORMAL, 0,0,0, DEFAULT_CHARSET,0,0, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");

    char buf[128];
    int  x = 20, y = 20, bar_w = cr.right - 160;

    //Title
    draw_label(hdc, big, CLR_BLUE, x, y, 300, "sysmon"); y += 30;

    //CPU
    draw_label(hdc, med, CLR_DIM, x, y, 80, "CPU");
    sprintf(buf, "%.1f%%", (double)g_stats.cpu);
    draw_label(hdc, med, CLR_TEXT, x + 80, y, 80, buf);
    draw_bar(hdc, x + 160, y + 3, bar_w, 14, g_stats.cpu);
    y += 30;

    //GPU
    draw_label(hdc, med, CLR_DIM, x, y, 80, "GPU");
    sprintf(buf, "%.1f%%", (double)g_stats.gpu);
    draw_label(hdc, med, CLR_TEXT, x + 80, y, 80, buf);
    draw_bar(hdc, x + 160, y + 3, bar_w, 14, g_stats.gpu);
    y += 30;

    //RAM Usage for those forsaken by RAM manufacturing companies
    draw_label(hdc, med, CLR_DIM, x, y, 80, "Memory");
    float mem_pct = g_stats.mem_total_mb > 0
        ? (float)g_stats.mem_used_mb / g_stats.mem_total_mb * 100.0f : 0;
    sprintf(buf, "%ld / %ld MB", g_stats.mem_used_mb, g_stats.mem_total_mb);
    draw_label(hdc, med, CLR_TEXT, x + 80, y, 200, buf);
    draw_bar(hdc, x + 160, y + 3, bar_w, 14, mem_pct);
    y += 30;

    //Disk
    draw_label(hdc, med, CLR_DIM, x, y, 80, "Disk C:");
    float disk_pct = g_stats.disk_total_gb > 0
        ? (float)g_stats.disk_used_gb / g_stats.disk_total_gb * 100.0f : 0;
    sprintf(buf, "%ld / %ld GB", g_stats.disk_used_gb, g_stats.disk_total_gb);
    draw_label(hdc, med, CLR_TEXT, x + 80, y, 200, buf);
    draw_bar(hdc, x + 160, y + 3, bar_w, 14, disk_pct);
    y += 30;

    //Network
    draw_label(hdc, med, CLR_DIM, x, y, 80, "Network");
    sprintf(buf, "RX %lu KB/s   TX %lu KB/s", g_stats.net_rx_kb, g_stats.net_tx_kb);
    draw_label(hdc, med, CLR_TEXT, x + 80, y, 400, buf);
    y += 40;

    //Divider
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 70));
    HPEN old = SelectObject(hdc, pen);
    MoveToEx(hdc, x, y, NULL); LineTo(hdc, cr.right - x, y);
    SelectObject(hdc, old); DeleteObject(pen);
    y += 10;

    //Process header
    draw_label(hdc, mono, CLR_DIM, x,       y, 60,  "PID");
    draw_label(hdc, mono, CLR_DIM, x + 70,  y, 200, "Name");
    draw_label(hdc, mono, CLR_DIM, x + 280, y, 120, "Memory");
    y += 22;

    //Process rows
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        //collect into array, sort by memory descending
        typedef struct { int pid; char name[64]; long mem_kb; } Proc;
        Proc procs[512]; int count = 0;

        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                if (count >= 512) break;
                HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                long mem = 0;
                if (ph) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(ph, &pmc, sizeof(pmc)))
                        mem = (long)(pmc.WorkingSetSize / 1024);
                    CloseHandle(ph);
                }
                procs[count].pid    = (int)pe.th32ProcessID;
                procs[count].mem_kb = mem;
                WideCharToMultiByte(CP_ACP, 0, pe.szExeFile, -1,
                                    procs[count].name, 63, NULL, NULL);
                count++;
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);

        //simple bubble-ish partial selection sort top 15
        int take = count < 15 ? count : 15;
        for (int i = 0; i < take; i++) {
            int best = i;
            for (int j = i+1; j < count; j++)
                if (procs[j].mem_kb > procs[best].mem_kb) best = j;
            Proc tmp = procs[i]; procs[i] = procs[best]; procs[best] = tmp;
        }

        for (int i = 0; i < take && y + 20 < cr.bottom - 10; i++) {
            // alternating row bg
            if (i % 2 == 0) {
                HBRUSH rb = CreateSolidBrush(RGB(28, 28, 42));
                RECT rr = { x - 4, y, cr.right - x + 4, y + 20 };
                FillRect(hdc, &rr, rb);
                DeleteObject(rb);
            }
            sprintf(buf, "%d", procs[i].pid);
            draw_label(hdc, mono, CLR_DIM,  x,       y, 60,  buf);
            draw_label(hdc, mono, CLR_TEXT, x + 70,  y, 200, procs[i].name);
            if (procs[i].mem_kb >= 1024)
                sprintf(buf, "%.1f MB", procs[i].mem_kb / 1024.0);
            else
                sprintf(buf, "%ld KB", procs[i].mem_kb);
            draw_label(hdc, mono, CLR_DIM,  x + 280, y, 120, buf);
            y += 20;
        }
    }

    BitBlt(hdc_screen, 0, 0, cr.right, cr.bottom, hdc, 0, 0, SRCCOPY);
    DeleteObject(bmp); DeleteDC(hdc);
    DeleteObject(big); DeleteObject(med); DeleteObject(mono);
    EndPaint(hwnd, &ps);
}

//Window procedure

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 1000, NULL);
        return 0;
    case WM_TIMER:
        update_stats();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_PAINT:
        paint(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

//Entry point

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
    (void)hPrev; (void)cmd;
    memset(&g_stats, 0, sizeof(g_stats));
    update_stats(); //first sample so bars aren't empty

    WNDCLASSW wc    = {0};
    wc.lpfnWndProc  = WndProc;
    wc.hInstance    = hInst;
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground= CreateSolidBrush(CLR_BG);
    wc.lpszClassName= L"Sysmon";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, L"Sysmon", L"sysmon",
                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                    700, 500, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
