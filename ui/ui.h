#ifndef UI_H
#define UI_H

#include <windows.h>
#include "../stats/stats.h"

#include "history.h"

typedef enum {
    TAB_PERFORMANCE = 0,
    TAB_PROCESSES,
    TAB_NETWORK,
    TAB_COUNT
} Tab;

typedef enum {
    METRIC_CPU = 0,
    METRIC_GPU,
    METRIC_MEMORY,
    METRIC_DISK,
    METRIC_NETWORK,
    METRIC_COUNT
} Metric;

typedef enum {
    SORT_MEMORY = 0,
    SORT_NAME,
    SORT_PID,
    SORT_COUNT
} ProcSort;

typedef struct {
    HFONT big;
    HFONT med;
    HFONT mono;
 
    Tab    active_tab;

    History hist[METRIC_COUNT];

    Metric selected_metric;

    ProcSort proc_sort;
} UiState;

void ui_init(UiState *ui);
void ui_destroy(UiState *ui);
void ui_push_history(UiState *ui, const Stats *s);
void paint(HWND hwnd, UiState *ui, const Stats *stats);

int  ui_hit_metric_card(int click_x, int click_y, int win_h);
int  ui_hit_proc_col(int click_x, int click_y);

int  ui_hit_tab(const UiState *ui, int click_x, int click_y);

#endif