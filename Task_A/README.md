# Task A: Process Information System Call

## Overview
This task implements a custom system call `proc_info` (syscall number 451) that provides process-specific and system-wide information from the Linux kernel.

## Modified Kernel Files

| File | Modification |
|------|-------------|
| `arch/x86/entry/syscalls/syscall_64.tbl` | Added entry: `451 common proc_info sys_proc_info` |
| `include/linux/syscalls.h` | Added declaration: `asmlinkage long sys_proc_info(pid_t pid, unsigned int flags, char __user *buffer, size_t size);` and forward declaration `struct pid_ctxt_switch;` |
| `Kbuild` (top-level) | Added: `obj-y += custom_syscalls/` |

## New Files

| File | Description |
|------|-------------|
| `custom_syscalls/proc_info.c` | Kernel-space implementation of the `proc_info` system call |
| `custom_syscalls/Makefile` | Build rule: `obj-y += proc_info.o` |
| `userspace/wrappers/proc_info.h` | Header file defining syscall number, flags, and wrapper function prototypes |
| `userspace/wrappers/proc_info.c` | Wrapper functions `get_proc_info()` and `get_system_info()` |
| `userspace/tests/test.c` | Test program covering both modes and error cases |
| `userspace/tests/Makefile` | Build rule for the test program |

## Syscall Interface

```c
long proc_info(pid_t pid, unsigned int flags, char __user *buffer, size_t size);
```

### Arguments
- `pid`: PID of the target process (used in process-specific mode); set to 0 for system-wide mode
- `flags`: Mode selector — `PROC_INFO_PID` (1) for process-specific, `PROC_INFO_SYSTEM` (2) for system-wide
- `buffer`: User-space buffer to receive the formatted result string
- `size`: Size of the user-space buffer

### Return Values
- `0` — Success
- `-EINVAL` — Invalid flags or arguments
- `-ESRCH` — No process found for the given PID
- `-EFAULT` — Invalid user-space buffer address
- `-ENOSPC` — Buffer too small

## Modes

### Process-Specific Mode (PROC_INFO_PID)
Returns the following fields (space-separated in buffer):
1. Parent Process ID
2. State (numeric)
3. Effective Priority
4. Scheduling Class (string, e.g., SCHED_NORMAL)
5. Number of Child Processes
6. Number of Sibling Processes

### System-Wide Mode (PROC_INFO_SYSTEM)
Returns the following fields (space-separated in buffer):
1. Total Number of Processes
2. Processes in TASK_RUNNING state
3. Processes in TASK_INTERRUPTIBLE state
4. Processes in TASK_UNINTERRUPTIBLE state
5. Processes in RT Class
6. Processes in Fair Class
7. Processes in CFS Runqueue
8. PID of process with minimum vruntime
9. Minimum vruntime value
10. Total load on CFS Runqueue

## How to Use the Library

### Step 1: Include the wrapper header
```c
#include "wrappers/proc_info.h"
#include "wrappers/proc_info.c"
```

### Step 2: Call the wrapper functions
```c
// Get info about a specific process
get_proc_info(1234);

// Get system-wide information
get_system_info();
```

### Step 3: Compile and run (must be on kernel 6.1.6)
```bash
cd userspace/tests
make
./test
```

## Design Details
- The syscall uses `pid_task(find_vpid(pid), PIDTYPE_PID)` to locate a process by PID
- All data is formatted into a kernel buffer using `snprintf`, then safely copied to user space via `copy_to_user`
- Child and sibling counts are obtained by iterating the `task->children` and `task->sibling` linked lists
- System-wide mode uses `for_each_process()` to iterate all processes and collects scheduler statistics from `task->se` (the scheduling entity)
- The `get_sched_class()` helper converts numeric scheduling policies to human-readable strings