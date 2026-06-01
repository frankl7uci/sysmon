#include "stats.h"
#include "../collectors/cpu.h"
#include "../collectors/gpu.h"
#include "../collectors/memory.h"
#include "../collectors/disk.h"
#include "../collectors/network.h"

void update_stats(Stats *s)
{
    s->cpu = get_cpu();
    s->gpu = get_gpu();

    get_memory(
        &s->mem_used_mb,
        &s->mem_total_mb);

    get_disk(
        &s->disk_used_gb,
        &s->disk_total_gb);

    get_network(
        &s->net_rx_kb,
        &s->net_tx_kb);
}