#include "disk.h"

#include "windows.h"

void get_disk(long *used, long *total)
{
    ULARGE_INTEGER free_b, total_b, dummy;
    GetDiskFreeSpaceExA("C:\\", &free_b, &total_b, &dummy);
    *total = (long)(total_b.QuadPart / (1024*1024*1024));
    *used  = (long)((total_b.QuadPart - free_b.QuadPart) / (1024*1024*1024));
}