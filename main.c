#include <windows.h>
#include <string.h>

#include "stats/stats.h"
#include "ui/ui.h"
#include "include/colors.h"

static Stats g_stats;

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wp,
    LPARAM lp)
{
    switch (msg) {

    case WM_CREATE:

        SetTimer(
            hwnd,
            1,
            1000,
            NULL
        );

        return 0;

    case WM_TIMER:

        update_stats(
            &g_stats
        );

        InvalidateRect(
            hwnd,
            NULL,
            FALSE
        );

        return 0;

    case WM_PAINT:

        paint(
            hwnd,
            &g_stats
        );

        return 0;

    case WM_DESTROY:

        PostQuitMessage(0);

        return 0;
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wp,
        lp
    );
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