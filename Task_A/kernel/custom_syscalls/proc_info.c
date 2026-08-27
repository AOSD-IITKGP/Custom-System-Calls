/*
(NOTE TO HADWIK AND YESHEETH): since this is code to kernel, we can't use our standard libs we use in general
We are going to use some kernel apis instead, refer the doc i shared
*/
#include <linux/kernel.h> // printk, snprintf
#include <linux/syscalls.h> // SYSCALL_DEFINE
#include <linux/sched.h> // task_struct, for_each_process
#include <linux/pid.h> // find_vpid, pid_task
#include <linux/uaccess.h> // copy_to_user, copy_from_user

/*
MACROS for flags to decode the mode
Convention: 1 - Single Process, 2 - All Processes Summary
All other modes are invalid
*/ 

#define PROC_INFO_PID 1
#define PROC_INFO_SYSTEM 2


/* 
(NOTE TO HADWIK AND YESHEETH): 
To understand this format to write syscalls, refer the doc I shared

Quick Reference for format: 
SYSCALL_DEFINE4(syscall_name, type1, name1, type2, name2, type3, name3, type4, name4)
*/

/*
(TODO YESHEETH) : Please verify if I mapped the strings and numbers correctly 
The following is the function that returns string for scheduling class
*/
static const char *get_sched_class(int policy){
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

SYSCALL_DEFINE4(proc_info, pid_t, pid, unsigned int, flags, char __user *, buffer, size_t, size) {  
    // Checks as mentioned in the assignment
    if(!(flags == 1 || flags == 2))
        return -EINVAL;

    if (!buffer)
        return -EFAULT;
    
    if (size <= 0)
        return -EINVAL;

    if(flags == PROC_INFO_PID){ // WE ARE IN THE SINGLE PROCESS MODE
        // let us find the process first
        struct task_struct* task;
        task = pid_task(find_vpid(pid), PIDTYPE_PID);

        if(!task) // task doesn't exist
            return -ESRCH;
        
        char kernel_buf[1024];
        int len = 0;

        // local variables to count child and siblings
        int n_child = 0;
        int n_sibling = 0; 

        /* NOTE TO HADWIK AND YESHEETH: 
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
            – Parent Process ID (pid)
            – State as a numeric value (__state)
            – Effective Priority (prio)
            – Scheduling class as a String (policy) => these are numbers tho.. need to convert to strings (TODO: YESHEETH)
            – Number of Child Processes (there is child list in task_Struct)
            – Number of Sibling Processes (there is sibling list in task_Struct)
        */

        // parent pid
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", task->parent->pid);

        // state
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%ld ", task->__state);

        // priority
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", task->prio);

        // scheduling class
        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%s ", get_sched_class(task->policy));

        // number of child processes (traverse through child list)
        struct list_head *p;
        list_for_each(p, &task->children) {
            n_child++;
        }

        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d ", n_child);

        // number of sibling process (traverse through sibling list)
        list_for_each(p, &task->sibling) {
            n_sibling++;
        }

        len += snprintf(kernel_buf + len, sizeof(kernel_buf) - len, "%d", n_sibling);

        // if user buffer size is too small
        if(size < len)
            return -ENOSPC;

        // copy it to user buffer 
        if (copy_to_user(buffer, kernel_buf, len + 1))
            return -EFAULT;

        return 0; // successful 
    }else{ // PROC_INFO_SYSTEM : WE ARE IN THE ALL PROCESSES MODE

    }
}

