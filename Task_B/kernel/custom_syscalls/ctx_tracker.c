#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "pid_ctxt_switch.h" // header file for pid_ctxt_switch struct


// define the structs as mentioned in the assignment
struct pid_node{
    pid_t pid;
    unsigned long ninvctxt;
    unsigned long nvctxt;
    struct list_head next_prev_list;
};

// the List that stores the pids to monitor
static LIST_HEAD(monitored_list);

/*
int sys_register(pid_t pid): adds the supplied PID to the tail of the monitored process list. 
The system call shall verify that the PID is valid and that the corresponding process exists before adding it to the list.
*/

SYSCALL_DEFINE1(register_pid, pid_t, pid){ // had to use register_pid since register is a keyword
    if(pid < 1)
        return -EINVAL;

    struct task_struct *task;
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    
    if(!task)
        return -ESRCH;

    // create new node for this pid and update the fields
    struct pid_node* new_node = kmalloc(sizeof(struct pid_node), GFP_KERNEL);
    if (!new_node)
        return -ENOSPC;

    new_node->pid = pid;
    new_node->ninvctxt = task -> nivcsw;
    new_node->nvctxt = task -> nvcsw;

    // add the above as tail to the list
    // NOTE (YESHEETH AND HADWIK): This is different from our usual linked list implementation, please look into this
    list_add_tail(&new_node->next_prev_list, &monitored_list);

    return 0;
}

/*
int sys_fetch(struct pid_ctxt_switch *stats): iterates through the monitored process list and obtains the cumulative number of voluntary 
and involuntary context switch events. The resulting values shall be copied into the user-provided stats structure.
*/

SYSCALL_DEFINE1(fetch, struct pid_ctxt_switch __user *, stats){
    if (!stats)
        return -EFAULT;

    struct pid_node* entry;
    /*
    list_for_each_entry(entry, &head, member_name)
    Where:
        entry — a pointer to YOUR struct type (not list_head)
        &head — the list head
        member_name — the name of the list_head field inside your struct
    */

    // the total involuntary and voluntary context switch tracker
    struct pid_ctxt_switch global_tracker = {0, 0};

    list_for_each_entry(entry, &monitored_list, next_prev_list){ 
        pid_t cur_pid = entry -> pid;
        struct task_struct *task;
        task = pid_task(find_vpid(cur_pid), PIDTYPE_PID);
    
        if(!task)
            continue; // move to the next process, if it is dead

        entry->ninvctxt = task->nivcsw;
        entry->nvctxt = task->nvcsw;

        global_tracker.tot_invctxt += task->nivcsw;
        global_tracker.tot_vctxt += task->nvcsw;
    }

    if (copy_to_user(stats, &global_tracker, sizeof(struct pid_ctxt_switch)))
        return -EFAULT;

    return 0;
}

/*
int sys_deregister(pid_t pid): searches the monitored process list for the sup
plied PID and removes the corresponding node from the list
*/

SYSCALL_DEFINE1(deregister, pid_t, pid){
    if(pid < 1)
        return -EINVAL;
    struct pid_node *entry, *tmp;

    list_for_each_entry_safe(entry, tmp, &monitored_list, next_prev_list){
        if(entry->pid == pid){
            // found it, now delete
            list_del(&entry->next_prev_list);
            kfree(entry);
            return 0;
        }
    }

    return -ESRCH;
}