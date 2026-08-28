#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "proc_info.h"

int get_proc_info(pid_t pid)
{
    char buffer[2048];
    long ret = syscall(__NR_proc_info, pid, PROC_INFO_PID, buffer, sizeof(buffer));

    if (ret != 0) {
        perror("proc_info (PID mode) failed");
        return ret;
    }

    int ppid, prio, n_child, n_sibling;
    long state;
    char sched_class[64];

    sscanf(buffer, "%d %ld %d %s %d %d",
           &ppid, &state, &prio, sched_class, &n_child, &n_sibling);

    printf("=== Process Info (PID: %d) ===\n", pid);
    printf("Parent PID:        %d\n", ppid);
    printf("State:             %ld\n", state);
    printf("Priority:          %d\n", prio);
    printf("Scheduling Class:  %s\n", sched_class);
    printf("Child Processes:   %d\n", n_child);
    printf("Sibling Processes: %d\n", n_sibling);

    return 0;
}

int get_system_info(void)
{
    char buffer[2048];
    long ret = syscall(__NR_proc_info, 0, PROC_INFO_SYSTEM, buffer, sizeof(buffer));

    if (ret != 0) {
        perror("proc_info (SYSTEM mode) failed");
        return ret;
    }

    int total, running, interruptible, uninterruptible;
    int rt, fair, cfs_rq, min_vruntime_pid;
    unsigned long long min_vruntime;
    unsigned long total_load;

    sscanf(buffer, "%d %d %d %d %d %d %d %d %llu %lu",
           &total, &running, &interruptible, &uninterruptible,
           &rt, &fair, &cfs_rq, &min_vruntime_pid,
           &min_vruntime, &total_load);

    printf("=== System-Wide Info ===\n");
    printf("Total Processes:       %d\n", total);
    printf("Running:               %d\n", running);
    printf("Interruptible:         %d\n", interruptible);
    printf("Uninterruptible:       %d\n", uninterruptible);
    printf("RT Class:              %d\n", rt);
    printf("Fair Class:            %d\n", fair);
    printf("In CFS Runqueue:       %d\n", cfs_rq);
    printf("Min Vruntime PID:      %d\n", min_vruntime_pid);
    printf("Min Vruntime:          %llu\n", min_vruntime);
    printf("Total CFS Load:        %lu\n", total_load);

    return 0;
}