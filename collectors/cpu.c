#include "cpu.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

float get_cpu(void)
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