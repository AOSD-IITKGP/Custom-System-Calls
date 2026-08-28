#ifndef _PID_CTXT_SWITCH_H
#define _PID_CTXT_SWITCH_H

struct pid_ctxt_switch {
    unsigned long tot_invctxt;
    unsigned long tot_vctxt;
};

#endif