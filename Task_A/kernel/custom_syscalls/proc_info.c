#include <linux/kernel.h> // printk, snprintf
#include <linux/syscalls.h> // SYSCALL_DEFINE
#include <linux/sched.h> // task_struct, for_each_process
#include <linux/pid.h> // find_vpid, pid_task
#include <linux/uaccess.h> // copy_to_user, copy_from_user
#include <linux/limits.h>    // ULLONG_MAX

#define PROC_INFO_PID 1
#define PROC_INFO_SYSTEM 2

static const char *get_sched_class(int policy)
{
    switch (policy) {
        case SCHED_NORMAL:   return "SCHED_NORMAL";
        case SCHED_FIFO:     return "SCHED_FIFO";
        case SCHED_RR:       return "SCHED_RR";
        case SCHED_BATCH:    return "SCHED_BATCH";
        case SCHED_IDLE:     return "SCHED_IDLE";
        case SCHED_DEADLINE: return "SCHED_DEADLINE";
        default:             return "UNKNOWN";
    }
}

/* 
SYSCALL_DEFINE4(syscall_name, type1, name1, type2, name2, type3, name3, type4, name4)
*/
SYSCALL_DEFINE4(proc_info, pid_t, pid, unsigned int, flags, char __user *, buffer, size_t, size) 
{  
    if(!(flags == 1 || flags == 2))
        return -EINVAL;

    if (!buffer)
        return -EFAULT;
    
    if (size <= 0)
        return -EINVAL;

    if(flags == PROC_INFO_PID)
    { 
        struct task_struct* task;
        task = pid_task(find_vpid(pid), PIDTYPE_PID);

        if(!task) // task doesn't exist
            return -ESRCH;
        
        char kernel_buf[2048];
        int len = 0;

        int n_child = 0;
        int n_sibling = 0; 

        /* 
        The following are the fields available in task struct
            • task->pid : The Process ID
            • task->parent->pid : The Parent’s PID
            • task->__state : Current state 
            • task->prio : Priority
            • task->policy : Scheduling policy 
            • task->nvcsw : Voluntary context switches
            • task->nivcsw : Involuntary context switches
            ....

        Out of the above fields we need the following:
            - Parent Process ID (pid)
            - State as a numeric value (__state)
            - Effective Priority (prio)
            - Scheduling class as a String (policy)
            - Number of Child Processes (there is child list in task_Struct)
            - Number of Sibling Processes (there is sibling list in task_Struct)
        */

        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", task->parent->pid);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%ld ", task->__state);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", task->prio);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%s ", get_sched_class(task->policy));

        // number of child processes (traverse through child list)
        struct list_head *p;
        list_for_each(p, &task->children) {
            n_child++;
        }

        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_child);

        // number of sibling process
        list_for_each(p, &task->sibling) {
            n_sibling++;
        }

        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d", n_sibling);

        // if user buffer size is too small
        if(size < len + 1)
            return -ENOSPC;

        if (copy_to_user(buffer, kernel_buf, len + 1))
            return -EFAULT;

        return 0; // successful 
    }
    else
    {
        char kernel_buf[2048];
        int len = 0;

        int total_procs = 0;
        int n_running = 0;
        int n_interruptible = 0;
        int n_uninterruptible = 0;
        int n_rt = 0;
        int n_fair = 0;
        int n_cfs_rq = 0;

        unsigned long long min_vruntime = ULLONG_MAX;
        pid_t min_vruntime_pid = 0;

        unsigned long total_load = 0;

        struct task_struct *task;

        for_each_process(task) 
        {
            total_procs++;

            // Count by state
            if (task->__state == TASK_RUNNING) n_running++;
            else if (task->__state == TASK_INTERRUPTIBLE) n_interruptible++;
            else if (task->__state == TASK_UNINTERRUPTIBLE) n_uninterruptible++;

            // Count by scheduling class
            if (task->policy == SCHED_FIFO || task->policy == SCHED_RR || task->policy == SCHED_DEADLINE) n_rt++;
            else if (task->policy == SCHED_NORMAL || task->policy == SCHED_BATCH || task->policy == SCHED_IDLE) n_fair++;

            // CFS runqueue: check if this task is on the runqueue AND is a fair-class task
            if (task->se.on_rq && (task->policy == SCHED_NORMAL || task->policy == SCHED_BATCH || task->policy == SCHED_IDLE)) 
            {
                n_cfs_rq++;
                total_load += task->se.load.weight;

                // Track the process with the smallest vruntime
                if (task->se.vruntime < min_vruntime) {
                    min_vruntime = task->se.vruntime;
                    min_vruntime_pid = task->pid;
                }
            }
        }

        // If no process was in CFS runqueue, set min_vruntime to 0
        if (min_vruntime == ULLONG_MAX)
            min_vruntime = 0;

        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", total_procs);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_running);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_interruptible);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_uninterruptible);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_rt);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_fair);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_cfs_rq);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", min_vruntime_pid);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%llu ", min_vruntime);
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%lu", total_load);

        // if user buffer size is too small
        if (size < len + 1)
            return -ENOSPC;

        if (copy_to_user(buffer, kernel_buf, len + 1))
            return -EFAULT;

        return 0;
    }
}

