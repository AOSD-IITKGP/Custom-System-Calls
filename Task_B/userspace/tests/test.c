#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "../wrappers/ctx_tracker.h"
#include "../wrappers/ctx_tracker.c"

// helper: spawn a busy child that does work and generates context switches
pid_t spawn_busy_child()
{
    pid_t pid = fork();
    if (pid == 0) {
        // child: do busy work
        volatile int x = 0;
        for (int i = 0; i < 10000000; i++)
            x += i;
        // then sleep to generate voluntary context switches
        for (int i = 0; i < 10; i++)
            usleep(1000);
        exit(0);
    }
    return pid;
}

int main()
{
    pid_t my_pid = getpid();
    struct pid_ctxt_switch stats;

    printf("=== Test 1: Register current process (PID %d) ===\n", my_pid);
    register_pid(my_pid);

    printf("\n=== Test 2: Register PID 1 (systemd) ===\n");
    register_pid(1);

    // spawn two busy children
    pid_t child1 = spawn_busy_child();
    pid_t child2 = spawn_busy_child();
    printf("\n=== Test 3: Register child processes (PID %d, %d) ===\n", child1, child2);
    register_pid(child1);
    register_pid(child2);

    // let children run and generate context switches
    sleep(1);

    printf("\n=== Test 4: Fetch with 4 processes monitored ===\n");
    fetch_ctx_switches(&stats);

    printf("\n=== Test 5: Deregister child1 (PID %d) ===\n", child1);
    deregister_pid(child1);

    printf("\n=== Test 6: Fetch with 3 processes monitored ===\n");
    fetch_ctx_switches(&stats);

    printf("\n=== Test 7: Deregister child2 (PID %d) ===\n", child2);
    deregister_pid(child2);

    printf("\n=== Test 8: Fetch with 2 processes monitored ===\n");
    fetch_ctx_switches(&stats);

    printf("\n=== Test 9: Deregister current process ===\n");
    deregister_pid(my_pid);

    printf("\n=== Test 10: Fetch with only PID 1 ===\n");
    fetch_ctx_switches(&stats);

    printf("\n=== Test 11: Deregister PID 1 ===\n");
    deregister_pid(1);

    // Error cases
    printf("\n=== Test 12: Invalid PID (-1) ===\n");
    register_pid(-1);

    printf("\n=== Test 13: Deregister non-existent PID ===\n");
    deregister_pid(99999);

    printf("\n=== Test 14: Double deregister ===\n");
    deregister_pid(my_pid);

    // wait for children to finish
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return 0;
}