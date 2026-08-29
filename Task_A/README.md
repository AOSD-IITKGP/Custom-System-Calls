# Task A: Process Information System Call

## Overview
We implemented a custom system call `proc_info` (#syscall number 451) that provides process-specific and system-wide information from the Linux kernel.

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
1. pid: PID of the process in process-specific mode; set to 0 in system-wide mode.
2. flags: mode selector as an unsigned integer.– Process-specific mode: PROC_INFO_PID– System-wide mode: PROC_INFO_SYSTEM
3. buffer: a char buffer that will store the extracted fields. You have to store all the fields that are extracted in this buffer, and then parse the buffer in the wrapper function. The buffer should contain the fields in the above mentioned order.
4. size: size of the user-space buffer.

### Return Values
1. 0: returned when the system call completes successfully and the requested fields are copied into buffer.
2. EINVAL: returned when flags contains unsupported bits, when more than one mode is selected, or when the supplied arguments are invalid.
3. ESRCH: returned when process-specific mode is selected and no process exists for the supplied pid.
4. EFAULT: returned when buffer is not a valid user-space address or the kernel cannot copy data to it.
5. ENOSPC: returned when size is too small to hold all fields in the required order.

## Modes

### Process-Specific Mode (PROC_INFO_PID)
Returns the following fields (note: space-separated in buffer):
1. Parent Process ID
2. State as a numeric value
3. Effective Priority
4. Scheduling class as a String
5. Number of Child Processes
6. Number of Sibling Processes

### System-Wide Mode (PROC_INFO_SYSTEM)
Returns the following fields (note: space-separated in buffer):
1. Total Number of Processes
2. Number of Processes in TASK_RUNNING state
3. Number of Processes in TASK_INTERRUPTIBLE state
4. Number of Processes in TASK_UNINTERRUPTIBLE state
5. Number of Processes in RT Class
6. Number of Processes in Fair Class
7. Number of Processes in CFS Runqueue
8. PID of Process with Minimum Vruntime Value
9. Minimum Vruntime Value
10. Total Load on CFS Runqueue

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
- System-wide mode iterates all processes and collects scheduler statistics from `task->se` (the scheduling entity)
- The `get_sched_class()` helper converts numebers to strings