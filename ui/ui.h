#ifndef UI_H
#define UI_H

#include <windows.h>
#include "../stats/stats.h"

typedef enum {
    TAB_PERFORMANCE = 0,
    TAB_PROCESSES,
    TAB_NETWORK,
    TAB_COUNT   /* keep last: used as array size / loop bound */
} Tab;

typedef struct {
    HFONT big;
    HFONT med;
    HFONT mono;
    Tab active_tab;
} UiState;

void ui_init(UiState *ui);
void ui_destroy(UiState *ui);
void paint(HWND hwnd, const UiState *ui, const Stats *stats);

int  ui_hit_tab(const UiState *ui, int click_x, int click_y);

#endif