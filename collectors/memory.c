#include "memory.h"

#include "windows.h"

void get_memory(long *used, long *total)
{
    MEMORYSTATUSEX m;
    m.dwLength = sizeof(m);
    GlobalMemoryStatusEx(&m);
    *total = (long)(m.ullTotalPhys  / (1024 * 1024));
    *used  = (long)((m.ullTotalPhys - m.ullAvailPhys) / (1024 * 1024));
}