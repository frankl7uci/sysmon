# sysmon
This is a simple project is a program to monitor your system's current state.

Run with this command to create the EXE file: 

gcc main.c stats/stats.c collectors/cpu.c collectors/gpu.c collectors/memory.c collectors/disk.c collectors/network.c processes/process_list.c ui/ui.c ui/drawing.c ui/history.c -o sysmon.exe -mwindows -lgdi32 -luser32 -liphlpapi -lpsapi -lpdh