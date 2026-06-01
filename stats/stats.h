#ifndef STATS_H
#define STATS_H

typedef struct {
    float cpu;
    float gpu;

    long mem_used_mb;
    long mem_total_mb;

    long disk_used_gb;
    long disk_total_gb;

    unsigned long net_rx_kb;
    unsigned long net_tx_kb;
} Stats;

void update_stats(Stats *s);

#endif