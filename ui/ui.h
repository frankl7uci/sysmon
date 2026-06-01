#ifndef UI_H
#define UI_H

#include <windows.h>
#include "../stats/stats.h"

void paint(
    HWND hwnd,
    const Stats *stats
);

#endif