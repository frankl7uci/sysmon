#include "process_list.h"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

typedef struct {
    int  pid;
    int  ppid;
    long mem_kb;
    char name[64];
} RawProc;

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

static int find_root(const RawProc *raw, int count, int pid)
{
    for (int iter = 0; iter < count; iter++) {
        int parent_idx = -1;
 
        for (int i = 0; i < count; i++) {
            if (raw[i].pid == pid) {
                int ppid = raw[i].ppid;
 
                for (int j = 0; j < count; j++) {
                    if (raw[j].pid == ppid &&
                        strcmp(raw[j].name, raw[i].name) == 0)
                    {
                        parent_idx = j;
                        break;
                    }
                }
                break;
            }
        }
 
        if (parent_idx == -1)
            return pid;
 
        pid = raw[parent_idx].pid;
    }
 
    return pid;
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

    RawProc *raw = (RawProc *)malloc(512 * sizeof(RawProc));
    if (!raw) { CloseHandle(snap); return 0; }

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

            raw[count].pid  = (int)pe.th32ProcessID;
            raw[count].ppid = (int)pe.th32ParentProcessID;
            raw[count].mem_kb = mem;

            WideCharToMultiByte(
                CP_ACP,
                0,
                pe.szExeFile,
                -1,
                raw[count].name, sizeof(raw[count].name),
                NULL,
                NULL
            );

            count++;

        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);

    Proc grouped[512];
    int  gcount = 0;
 
    for (int i = 0; i < count; i++) {
        int root_pid = find_root(raw, count, raw[i].pid);
 
        int found = -1;
        for (int g = 0; g < gcount; g++) {
            if (grouped[g].pid == root_pid) {
                found = g;
                break;
            }
        }
 
        if (found >= 0) {
            grouped[found].mem_kb += raw[i].mem_kb;
        } else {
            if (gcount >= 512) continue;
 
            grouped[gcount].pid    = root_pid;
            grouped[gcount].mem_kb = raw[i].mem_kb;
 
            const char *root_name = raw[i].name;
            for (int j = 0; j < count; j++) {
                if (raw[j].pid == root_pid) {
                    root_name = raw[j].name;
                    break;
                }
            }
            strncpy(grouped[gcount].name, root_name,
                    sizeof(grouped[gcount].name) - 1);
            grouped[gcount].name[sizeof(grouped[gcount].name) - 1] = '\0';
 
            gcount++;
        }
    }
 
    free(raw);

    int take = (gcount < max_count)
        ? gcount 
        : max_count;

    sort_by_memory(
        grouped,
        gcount,
        take
    );

    for (int i = 0; i < take; i++) {
        list[i] = grouped[i];
    }

    return take;
}