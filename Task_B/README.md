# Task B: Context Switch Tracker

## Overview
We implemented 3 custom system calls, `sys_register_pid` (452), `sys_fetch` (453), and `sys_deregister` (454), by maintaining a monitored process list in the Linux kernel and tracking cumulative voluntary and involuntary context switch events across registered processes and their threads.

## Modified Kernel Files

| File | Modification |
|------|-------------|
| `arch/x86/entry/syscalls/syscall_64.tbl` | Added entries 452, 453, 454 for `register_pid`, `fetch`, and `deregister` |
| `include/linux/syscalls.h` | Added declarations for `sys_register_pid`, `sys_fetch`, `sys_deregister` |
| `Kbuild` (top-level) | Added: `obj-y += custom_syscalls/` |

## New Files

| File | Description |
|------|-------------|
| `custom_syscalls/ctx_tracker.c` | Kernel-space implementation of all three system calls with mutex synchronization |
| `custom_syscalls/pid_ctxt_switch.h` | Shared header defining `struct pid_ctxt_switch` (used in both kernel and userspace) |
| `custom_syscalls/Makefile` | Build rule: `obj-y += ctx_tracker.o` |
| `userspace/wrappers/ctx_tracker.h` | Header file defining syscall numbers and wrapper function prototypes |
| `userspace/wrappers/ctx_tracker.c` | Wrapper functions `register_pid()`, `fetch_ctx_switches()`, `deregister_pid()` |
| `userspace/tests/test.c` | Test program covering registration, fetching, deregistration, multi-core safety, and error cases |
| `userspace/tests/Makefile` | Build rule for the test program |

---

## Syscall Interfaces

### `sys_register_pid` (452)
```c
long sys_register_pid(pid_t pid);
```
Adds the supplied PID to the tail of the monitored process list.
*(Note: Named `register_pid` to avoid collision with C runtime symbols).*

**Return Values:**
- `0` — Success
- `-EINVAL` — PID is less than 1
- `-ESRCH` — No process exists for the given PID
- `-ENOSPC` — Insufficient kernel memory to create a new node
- `-EEXIST` — PID is already present in the monitored list (duplicate registration)

---

### `sys_fetch` (453)
```c
long sys_fetch(struct pid_ctxt_switch __user *stats);
```
Iterates through the monitored process list, accumulates cumulative voluntary and involuntary context switch counts across each registered process and all its threads, and copies the totals to the user-supplied structure.

**Return Values:**
- `0` — Success
- `-EFAULT` — Invalid user-space destination address

---

### `sys_deregister` (454)
```c
long sys_deregister(pid_t pid);
```
Searches the monitored process list for the supplied PID, unlinks the node, and frees its kernel memory.

**Return Values:**
- `0` — Success
- `-EINVAL` — PID is less than 1
- `-ESRCH` — PID not found in the monitored list

---

## Data Structures

### `struct pid_node` (kernel only)
```c
struct pid_node {
    pid_t pid;                        // Monitored process PID
    unsigned long ninvctxt;           // Involuntary context switches
    unsigned long nvctxt;             // Voluntary context switches
    struct list_head next_prev_list;  // Kernel doubly linked list connector
};
```

### `struct pid_ctxt_switch` (shared between kernel and userspace)
```c
struct pid_ctxt_switch {
    unsigned long tot_invctxt;   // Total involuntary context switches
    unsigned long tot_vctxt;     // Total voluntary context switches
};
```

---

## Design Choices & Implementation Details

### 1. Multi-Core Synchronization & Mutex Locking
- `monitored_list` is a global kernel data structure accessible across all CPU cores simultaneously.
- To prevent race conditions, pointer corruption, and kernel panics when multiple cores concurrently invoke `register_pid`, `deregister`, or `fetch`, all list access is synchronized using a dedicated kernel mutex:
  ```c
  static DEFINE_MUTEX(monitored_list_lock);
  ```
- **Optimized Lock Scope**: Memory allocation (`kmalloc`) in `register_pid` and user-space copying (`copy_to_user`) in `fetch` are performed outside the critical section to minimize lock contention and lock hold time.

### 2. Duplicate PID Prevention
- Before appending a new process node, `sys_register_pid` traverses `monitored_list` under the mutex to verify uniqueness.
- If the PID is already present, it immediately unlocks the mutex, frees the allocated node via `kfree(new_node)` to prevent memory leaks, and returns `-EEXIST`.
- **Benefits**: Guarantees that `fetch` never double-counts context switches for a PID, and `deregister` reliably removes the single clean entry.

### 3. Thread-Group Context Switch Aggregation with RCU
- A process may have multiple threads contributing to overall CPU context switching.
- In `sys_fetch`, the code traverses all threads of each monitored process using `while_each_thread()` inside `rcu_read_lock()` and `rcu_read_unlock()`. This ensures thread descriptors are protected against concurrent termination while summing involuntary (`thread->nivcsw`) and voluntary (`thread->nvcsw`) switches.

### 4. Safe Deletion & Memory Cleanup
- `sys_deregister` uses `list_for_each_entry_safe()` to enable safe deletion while iterating, unlinks the node with `list_del()`, unlocks the mutex, and frees the kernel node using `kfree()`.

---

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

### Step 3: Compile and run
```bash
cd userspace/tests
make
./test
```