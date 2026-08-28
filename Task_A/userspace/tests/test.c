#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "../wrappers/proc_info.h"
#include "../wrappers/proc_info.c"

int main(int argc, char *argv[])
{
    printf("--- Test 1: Current process info ---\n");
    get_proc_info(getpid());

    printf("\n--- Test 2: Init process (PID 1) ---\n");
    get_proc_info(1);

    printf("\n--- Test 3: System-wide info ---\n");
    get_system_info();

    printf("\n--- Test 4: Invalid PID ---\n");
    char buf[2048];
    long ret = syscall(451, 99999, PROC_INFO_PID, buf, sizeof(buf));
    printf("Invalid PID returned: %ld (expected -1, errno=ESRCH)\n", ret);
    if (ret == -1)
        perror("Error");

    printf("\n--- Test 5: Invalid flags ---\n");
    ret = syscall(451, 1, 99, buf, sizeof(buf));
    printf("Invalid flags returned: %ld (expected -1, errno=EINVAL)\n", ret);
    if (ret == -1)
        perror("Error");

    printf("\n--- Test 6: Buffer too small ---\n");
    ret = syscall(451, 1, PROC_INFO_PID, buf, 1);
    printf("Small buffer returned: %ld (expected -1, errno=ENOSPC)\n", ret);
    if (ret == -1)
        perror("Error");

    return 0;
}   