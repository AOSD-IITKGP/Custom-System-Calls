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
1. `pid`: PID of the process in process-specific mode; set to 0 in system-wide mode.
2. `flags`: Mode selector as an unsigned integer:
   - Process-specific mode: `PROC_INFO_PID` (1)
   - System-wide mode: `PROC_INFO_SYSTEM` (2)
3. `buffer`: A char buffer that will store the extracted fields formatted as a space-separated string.
4. `size`: Size of the user-space buffer.

### Return Values
1. `0`: Returned when the system call completes successfully and the requested fields are copied into `buffer`.
2. `-EINVAL`: Returned when `flags` contains unsupported bits/values, or when supplied arguments are invalid.
3. `-ESRCH`: Returned when process-specific mode is selected and no process exists for the supplied `pid`.
4. `-EFAULT`: Returned when `buffer` is not a valid user-space address or `copy_to_user` fails.
5. `-ENOSPC`: Returned when `size` is too small to hold all formatted fields.

---

## Modes

### Process-Specific Mode (`PROC_INFO_PID`)
Returns the following fields (space-separated):
1. Parent Process ID (`task->parent->pid`)
2. State as a numeric value (`task->__state`)
3. Effective Priority (`task->prio`)
4. Scheduling class as a String (`get_sched_class(task->policy)`)
5. Number of Child Processes (`task->children`)
6. Number of Sibling Processes (`task->sibling`)

### System-Wide Mode (`PROC_INFO_SYSTEM`)
Returns the following fields (space-separated):
1. Total Number of Processes
2. Number of Processes in `TASK_RUNNING` state
3. Number of Processes in `TASK_INTERRUPTIBLE` state
4. Number of Processes in `TASK_UNINTERRUPTIBLE` state
5. Number of Processes in RT Class
6. Number of Processes in Fair Class
7. Number of Processes in CFS Runqueue
8. PID of Process with Minimum Vruntime Value
9. Minimum Vruntime Value
10. Total Load on CFS Runqueue

---

## Design Choices & Implementation Details

### 1. Scheduling Class Classification
- **Fair Class (`n_fair`)**: Categorizes `SCHED_NORMAL`, `SCHED_BATCH`, and `SCHED_IDLE`. In the Linux kernel, processes assigned `SCHED_IDLE` are scheduled under the CFS scheduler (`fair_sched_class`) with a minimal priority weight (`WEIGHT_IDLEPRIO = 3`), maintain `vruntime`, and reside in CFS runqueues.
- **Real-Time Class (`n_rt`)**: Categorizes `SCHED_FIFO`, `SCHED_RR`, and `SCHED_DEADLINE`. Grouping `SCHED_DEADLINE` under real-time captures all deadline-driven real-time tasks alongside POSIX real-time tasks, ensuring complete coverage such that:
  $$\text{n\_rt} + \text{n\_fair} = \text{total\_procs}$$

### 2. Multi-Core CFS Aggregation
- In Linux SMP systems, each CPU core maintains its own dedicated `cfs_rq` to eliminate global lock contention.
- The syscall iterates over all global processes via `for_each_process(task)`. For every runnable CFS task (`task->se.on_rq`), it aggregates `task->se.load.weight` into `total_load` (the system-wide CFS capacity demand) and tracks the global minimum `vruntime` across all cores.

### 3. Kernel Process State Tracking
- Linux kernel 5.14+ replaced `task->state` with `task->__state`. The implementation checks `task->__state` to accurately count `TASK_RUNNING`, `TASK_INTERRUPTIBLE`, and `TASK_UNINTERRUPTIBLE` processes.

### 4. Single Buffer Kernel-to-Userspace Transfer
- Instead of multiple syscalls or individual struct field copies, all extracted fields are formatted into a single 2048-byte kernel buffer using `snprintf()` and transferred via a single `copy_to_user()` call. Userspace wrapper functions then parse this string into respective variables.

---

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

### Step 3: Compile and run
```bash
cd userspace/tests
make
./test
```