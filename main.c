#include <windows.h>
#include <string.h>
#include <stdio.h>

#include <psapi.h>
#include <shellapi.h>

#include "stats/stats.h"
#include "ui/ui.h"
#include "include/colors.h"

#define IDM_END_TASK  1001

#define IDM_OPEN_LOCATION  1002

static Stats g_stats;
static UiState g_ui;

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wp,
    LPARAM lp)
{
    switch (msg) {
 
    case WM_CREATE:
        ui_init(&g_ui);
        SetTimer(hwnd, 1, 1000, NULL);
        return 0;
 
    case WM_TIMER:
        update_stats(&g_stats);
        ui_push_history(&g_ui, &g_stats);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
 
    case WM_PAINT:
        paint(hwnd, &g_ui, &g_stats);
        return 0;

    case WM_MOUSEMOVE: {
        if (g_ui.active_tab == TAB_PROCESSES) {
            int y = (int)HIWORD(lp);
            int row = ui_hit_proc_row(y);
            if (row != g_ui.hovered_row) {
                g_ui.hovered_row = row;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
 
    case WM_LBUTTONDOWN: {
        int x = (int)LOWORD(lp);
        int y = (int)HIWORD(lp);

        int tab = ui_hit_tab(&g_ui, x, y);
        if (tab >= 0) {
            g_ui.active_tab    = (Tab)tab;
            g_ui.selected_row  = -1;
            g_ui.hovered_row   = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_ui.active_tab == TAB_PERFORMANCE) {
            int m = ui_hit_metric_card(x, y, 0);
            if (m >= 0) {
                g_ui.selected_metric = (Metric)m;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }

        if (g_ui.active_tab == TAB_PROCESSES) {
            int col = ui_hit_proc_col(x, y);
            if (col >= 0) {
                g_ui.proc_sort = (ProcSort)col;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            int row = ui_hit_proc_row(y);
            if (row >= 0 && row < g_ui._proc_count) {
                g_ui.selected_row = row;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }

        return 0;
    }

    case WM_RBUTTONDOWN: {
        if (g_ui.active_tab != TAB_PROCESSES)
            return 0;

        int y = (int)HIWORD(lp);
        int row = ui_hit_proc_row(y);
        if (row < 0 || row >= g_ui._proc_count)
            return 0;

        g_ui.selected_row = row;
        InvalidateRect(hwnd, NULL, FALSE);

        HMENU menu = CreatePopupMenu();
        char label[128];
        sprintf(label, "End Task  (%s)", g_ui._procs[row].name);
        AppendMenuA(menu, MF_STRING, IDM_END_TASK, label);
        AppendMenuA(menu, MF_STRING, IDM_OPEN_LOCATION, "Open File Location");

        POINT pt = { (int)LOWORD(lp), y };
        ClientToScreen(hwnd, &pt);

        int cmd = (int)TrackPopupMenu(
            menu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
            pt.x, pt.y,
            0, hwnd, NULL);

        DestroyMenu(menu);

        if (cmd == IDM_END_TASK) {
            int pid = g_ui._procs[row].pid;
            HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
            if (ph) {
                TerminateProcess(ph, 1);
                CloseHandle(ph);
            }
            g_ui.selected_row = -1;
            update_stats(&g_stats);
            ui_push_history(&g_ui, &g_stats);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (cmd == IDM_OPEN_LOCATION) {
            int pid = g_ui._procs[row].pid;
            HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                    FALSE, (DWORD)pid);
            if (ph) {
                char path[MAX_PATH] = {0};
                if (GetModuleFileNameExA(ph, NULL, path, MAX_PATH)) {
                    char cmd_buf[MAX_PATH + 32];
                    sprintf(cmd_buf, "/select,\"%s\"", path);
                    ShellExecuteA(NULL, "open", "explorer.exe",
                                  cmd_buf, NULL, SW_SHOWNORMAL);
                }
                CloseHandle(ph);
            }
        }

        return 0;
    }

    case WM_MOUSELEAVE:
        g_ui.hovered_row = -1;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
 
    case WM_DESTROY:
        ui_destroy(&g_ui);
        PostQuitMessage(0);
        return 0;
    }
 
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR cmd,
    int show)
{
    (void)hPrev;
    (void)cmd;

    memset(
        &g_stats,
        0,
        sizeof(g_stats)
    );

    update_stats(
        &g_stats
    );

    WNDCLASSW wc = {0};

    wc.lpfnWndProc =
        WndProc;

    wc.hInstance =
        hInst;

    wc.hCursor =
        LoadCursor(
            NULL,
            IDC_ARROW
        );

    wc.hbrBackground =
        CreateSolidBrush(
            CLR_BG
        );

    wc.lpszClassName =
        L"Sysmon";

    RegisterClassW(
        &wc
    );

    HWND hwnd =
        CreateWindowExW(
            0,
            L"Sysmon",
            L"sysmon",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1000,
            600,
            NULL,
            NULL,
            hInst,
            NULL
        );

    ShowWindow(
        hwnd,
        show
    );

    UpdateWindow(
        hwnd
    );

    MSG msg;

    while (
        GetMessageW(
            &msg,
            NULL,
            0,
            0
        )
    )
    {
        TranslateMessage(
            &msg
        );

        DispatchMessageW(
            &msg
        );
    }

    return (int)msg.wParam;
}