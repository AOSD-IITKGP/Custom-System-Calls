# Task B: Context Switch Tracker

## Overview
We implemented 3 custom system calls, `sys_register_pid` (452), `sys_fetch` (453), and `sys_deregister` (454), by maintaining a monitored process list and tracking cumulative voluntary and involuntary context switch events across registered processes and their threads.

## Modified Kernel Files

| File | Modification |
|------|-------------|
| `arch/x86/entry/syscalls/syscall_64.tbl` | Added entries 452, 453, 454 for register_pid, fetch, and deregister |
| `include/linux/syscalls.h` | Added declarations for `sys_register_pid`, `sys_fetch`, `sys_deregister` |
| `Kbuild` (top-level) | Added: `obj-y += custom_syscalls/` |

## New Files

| File | Description |
|------|-------------|
| `custom_syscalls/ctx_tracker.c` | Kernel-space implementation of all three system calls |
| `custom_syscalls/pid_ctxt_switch.h` | Shared header defining `struct pid_ctxt_switch` (used in both kernel and userspace) |
| `custom_syscalls/Makefile` | Build rule: `obj-y += ctx_tracker.o` |
| `userspace/wrappers/ctx_tracker.h` | Header file defining syscall numbers and wrapper function prototypes |
| `userspace/wrappers/ctx_tracker.c` | Wrapper functions `register_pid()`, `fetch_ctx_switches()`, `deregister_pid()` |
| `userspace/tests/test.c` | Test program covering registration, fetching, deregistration, and error cases |
| `userspace/tests/Makefile` | Build rule for the test program |

## Syscall Interfaces

### sys_register_pid (452)
```c
long sys_register_pid(pid_t pid);
```
Adds the given PID to the tail of the monitored process list.
NOTE: Had to use register_pid() instead of register() to avoid conflict with the standard library function.

**Return Values:**
- `0` — Success
- `-EINVAL` — PID is less than 1
- `-ESRCH` — No process exists for the given PID
- `-ENOSPC` — Insufficient kernel memory to create a new node

### sys_fetch (453)
```c
long sys_fetch(struct pid_ctxt_switch __user *stats);
```
Iterates through the monitored list, accumulates voluntary and involuntary context switches from each monitored process and all its threads, and copies the totals to user space.

**Return Values:**
- `0` — Success
- `-EFAULT` — Invalid user-space address

### sys_deregister (454)
```c
long sys_deregister(pid_t pid);
```
Removes the given PID from the monitored process list and frees the associated memory.

**Return Values:**
- `0` — Success
- `-EINVAL` — PID is less than 1
- `-ESRCH` — PID not found in the monitored list

## Data Structures

### struct pid_node (kernel only)
```c
struct pid_node {
    pid_t pid;                        // Monitored process PID
    unsigned long ninvctxt;           // Involuntary context switches
    unsigned long nvctxt;             // Voluntary context switches
    struct list_head next_prev_list;  // Kernel linked list connector
};
```

### struct pid_ctxt_switch (shared between kernel and userspace)
```c
struct pid_ctxt_switch {
    unsigned long tot_invctxt;   // Total involuntary context switches
    unsigned long tot_vctxt;     // Total voluntary context switches
};
```

## How to Use the Library

### Step 1: Include the wrapper header
```c
#include "wrappers/ctx_tracker.h"
#include "wrappers/ctx_tracker.c"
```

### Step 2: Call the wrapper functions
```c
// Register a process for monitoring
register_pid(1234);

// Fetch cumulative context switches across all monitored processes
struct pid_ctxt_switch stats;
fetch_ctx_switches(&stats);
printf("Voluntary: %lu, Involuntary: %lu\n", stats.tot_vctxt, stats.tot_invctxt);

// Deregister a process
deregister_pid(1234);
```

### Step 3: Compile and run (must be on kernel 6.1.6)
```bash
cd userspace/tests
make
./test
```

## Design Details
- The monitored list is implemented using the Linux kernel's intrusive doubly linked list (`include/linux/list.h`) with a statically defined head: `static LIST_HEAD(monitored_list)`
- New nodes are allocated with `kmalloc(sizeof(struct pid_node), GFP_KERNEL)` and added to the tail using `list_add_tail()`
- `sys_fetch` iterates all threads of each monitored process using `while_each_thread()` to capture context switches from the entire thread group, not just the main thread
- `sys_deregister` uses `list_for_each_entry_safe()` for safe deletion during iteration, and frees memory with `kfree()`
- The `pid_ctxt_switch.h` header is shared between kernel and userspace to ensure both sides agree on the struct layout
- If a monitored process has died between registration and fetch, it is silently skipped (not removed from the list)