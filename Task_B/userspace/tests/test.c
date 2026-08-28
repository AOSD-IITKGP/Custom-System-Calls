#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "../wrappers/ctx_tracker.h"
#include "../wrappers/ctx_tracker.c"

int main()
{
    pid_t my_pid = getpid();
    struct pid_ctxt_switch stats;

    printf("--- Test 1: Register current process ---\n");
    register_pid(my_pid);

    printf("\n--- Test 2: Register PID 1 ---\n");
    register_pid(1);

    printf("\n--- Test 3: Fetch context switches ---\n");
    fetch_ctx_switches(&stats);

    printf("\n--- Test 4: Deregister current process ---\n");
    deregister_pid(my_pid);

    printf("\n--- Test 5: Fetch after deregister ---\n");
    fetch_ctx_switches(&stats);

    printf("\n--- Test 6: Deregister PID 1 ---\n");
    deregister_pid(1);

    printf("\n--- Test 7: Invalid PID ---\n");
    register_pid(-1);

    printf("\n--- Test 8: Deregister non-existent PID ---\n");
    deregister_pid(99999);

    return 0;
}