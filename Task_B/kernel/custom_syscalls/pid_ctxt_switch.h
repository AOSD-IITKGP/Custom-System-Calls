#ifndef _PID_CTXT_SWITCH_H
#define _PID_CTXT_SWITCH_H

struct pid_ctxt_switch {
    unsigned long tot_invctxt;
    unsigned long tot_vctxt;
};

#endif

// used this in kernel/custom_syscalls/ctx_tracker.c and userspace/wrappers/ctx_tracker.c