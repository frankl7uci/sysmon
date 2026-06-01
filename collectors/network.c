#include "network.h"

#include "windows.h"
#include <tlhelp32.h>
#include <iphlpapi.h>

void get_network(unsigned long *rx_kb, unsigned long *tx_kb)
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