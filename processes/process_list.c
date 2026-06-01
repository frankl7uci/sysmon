#include "process_list.h"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

static void sort_by_memory(Proc *procs, int count, int take)
{
    for (int i = 0; i < take; i++) {
        int best = i;

        for (int j = i + 1; j < count; j++) {
            if (procs[j].mem_kb > procs[best].mem_kb)
                best = j;
        }

        Proc temp = procs[i];
        procs[i] = procs[best];
        procs[best] = temp;
    }
}

int get_top_processes(Proc *list, int max_count)
{
    if (!list || max_count <= 0)
        return 0;

    HANDLE snap = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );

    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    Proc all_procs[512];
    int count = 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe)) {

        do {

            if (count >= 512)
                break;

            long mem = 0;

            HANDLE ph = OpenProcess(
                PROCESS_QUERY_INFORMATION |
                PROCESS_VM_READ,
                FALSE,
                pe.th32ProcessID
            );

            if (ph) {

                PROCESS_MEMORY_COUNTERS pmc;

                if (GetProcessMemoryInfo(
                        ph,
                        &pmc,
                        sizeof(pmc)))
                {
                    mem = (long)(
                        pmc.WorkingSetSize / 1024
                    );
                }

                CloseHandle(ph);
            }

            all_procs[count].pid =
                (int)pe.th32ProcessID;

            all_procs[count].mem_kb =
                mem;

            WideCharToMultiByte(
                CP_ACP,
                0,
                pe.szExeFile,
                -1,
                all_procs[count].name,
                sizeof(all_procs[count].name),
                NULL,
                NULL
            );

            count++;

        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);

    int take = (count < max_count)
        ? count
        : max_count;

    sort_by_memory(
        all_procs,
        count,
        take
    );

    for (int i = 0; i < take; i++) {
        list[i] = all_procs[i];
    }

    return take;
}