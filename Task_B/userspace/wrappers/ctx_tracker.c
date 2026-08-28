#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "ctx_tracker.h"

int register_pid(pid_t pid)
{
    long ret = syscall(__NR_register_pid, pid);
    if (ret != 0) {
        perror("register_pid failed");
        return ret;
    }
    printf("Registered PID %d successfully\n", pid);
    return 0;
}

int fetch_ctx_switches(struct pid_ctxt_switch *stats)
{
    long ret = syscall(__NR_fetch, stats);
    if (ret != 0) {
        perror("fetch failed");
        return ret;
    }
    printf("Voluntary Context Switches:   %lu\n", stats->tot_vctxt);
    printf("Involuntary Context Switches: %lu\n", stats->tot_invctxt);
    return 0;
}

int deregister_pid(pid_t pid)
{
    long ret = syscall(__NR_deregister, pid);
    if (ret != 0) {
        perror("deregister failed");
        return ret;
    }
    printf("Deregistered PID %d successfully\n", pid);
    return 0;
}