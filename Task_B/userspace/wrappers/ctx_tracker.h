#ifndef _CTX_TRACKER_H
#define _CTX_TRACKER_H

#include <sys/types.h>
#include "../../kernel/custom_syscalls/pid_ctxt_switch.h"

#define __NR_register_pid 452
#define __NR_fetch 453
#define __NR_deregister 454

int register_pid(pid_t pid);
int fetch_ctx_switches(struct pid_ctxt_switch *stats);
int deregister_pid(pid_t pid);

#endif