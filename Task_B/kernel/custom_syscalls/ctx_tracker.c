#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched/signal.h>
#include <linux/mutex.h>
#include "pid_ctxt_switch.h"

struct pid_node{
    pid_t pid;
    unsigned long ninvctxt;
    unsigned long nvctxt;
    struct list_head next_prev_list;
};

// the List that stores the pids to monitor and its mutex lock
static LIST_HEAD(monitored_list);
static DEFINE_MUTEX(monitored_list_lock);

/*
int sys_register_pid(pid_t pid): adds the supplied PID to the tail of the monitored process list. 
The system call shall verify that the PID is valid and that the corresponding process exists before adding it to the list.
*/
SYSCALL_DEFINE1(register_pid, pid_t, pid)
{
    if (pid < 1)
        return -EINVAL;

    struct task_struct *task;
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task)
        return -ESRCH;

    // Allocate memory before acquiring the lock
    struct pid_node *new_node = kmalloc(sizeof(struct pid_node), GFP_KERNEL);
    if (!new_node)
        return -ENOSPC;

    new_node->pid = pid;
    new_node->ninvctxt = task->nivcsw;
    new_node->nvctxt = task->nvcsw;

    mutex_lock(&monitored_list_lock);

    // 1. Check if PID is already registered to avoid duplicate entries
    struct pid_node *entry;

    /*
    list_for_each_entry(entry, &head, member_name)
    Where:
        entry — a pointer to YOUR struct type (not list_head)
        &head — the list head
        member_name — the name of the list_head field inside your struct
    */

    list_for_each_entry(entry, &monitored_list, next_prev_list) {
        if (entry->pid == pid) {
            mutex_unlock(&monitored_list_lock);
            kfree(new_node);
            return -EEXIST; // PID is already in the list
        }
    }

    // 2. Add to the tail of the list
    list_add_tail(&new_node->next_prev_list, &monitored_list);

    mutex_unlock(&monitored_list_lock);

    return 0;
}


/*
int sys_fetch(struct pid_ctxt_switch *stats): iterates through the monitored process list and obtains the cumulative number of voluntary 
and involuntary context switch events. The resulting values shall be copied into the user-provided stats structure.
*/
SYSCALL_DEFINE1(fetch, struct pid_ctxt_switch __user *, stats)
{
    if (!stats)
        return -EFAULT;

    struct pid_node *entry;
    struct pid_ctxt_switch global_tracker = {0, 0};

    mutex_lock(&monitored_list_lock);

    list_for_each_entry(entry, &monitored_list, next_prev_list) { 
        pid_t cur_pid = entry->pid;

        rcu_read_lock();
        struct task_struct *task = pid_task(find_vpid(cur_pid), PIDTYPE_PID);
        if (!task) {
            rcu_read_unlock();
            continue;
        }

        // loop through the main process AND all its threads
        struct task_struct *thread = task; // Fix done by Hadwik
        unsigned long proc_nivcsw = 0;
        unsigned long proc_nvcsw = 0;

        do {
            proc_nivcsw += thread->nivcsw;
            proc_nvcsw += thread->nvcsw;
        } while_each_thread(task, thread);

        rcu_read_unlock();

        entry->ninvctxt = proc_nivcsw;
        entry->nvctxt = proc_nvcsw;
        global_tracker.tot_invctxt += proc_nivcsw;
        global_tracker.tot_vctxt += proc_nvcsw;
    }

    mutex_unlock(&monitored_list_lock);

    // send this to user struct
    if (copy_to_user(stats, &global_tracker, sizeof(struct pid_ctxt_switch)))
        return -EFAULT;

    return 0;
}

/*
int sys_deregister(pid_t pid): searches the monitored process list for the supplied PID and removes the corresponding node from the list
*/
SYSCALL_DEFINE1(deregister, pid_t, pid)
{
    if (pid < 1)
        return -EINVAL;

    struct pid_node *entry, *tmp;

    mutex_lock(&monitored_list_lock);

    // loop through it and delete the pid safely
    list_for_each_entry_safe(entry, tmp, &monitored_list, next_prev_list) {
        if (entry->pid == pid) {
            list_del(&entry->next_prev_list);
            mutex_unlock(&monitored_list_lock);
            kfree(entry);
            return 0;
        }
    }

    mutex_unlock(&monitored_list_lock);

    return -ESRCH;
}