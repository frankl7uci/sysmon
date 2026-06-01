#ifndef PROCESS_LIST_H
#define PROCESS_LIST_H

typedef struct {
    int pid;
    char name[64];
    long mem_kb;
} Proc;

int get_top_processes(
    Proc *list,
    int max_count);

#endif