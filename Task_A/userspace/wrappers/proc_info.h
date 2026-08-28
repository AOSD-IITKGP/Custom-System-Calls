#ifndef _PROC_INFO_H
#define _PROC_INFO_H

#include <sys/types.h>

#define PROC_INFO_PID 1
#define PROC_INFO_SYSTEM 2
#define __NR_proc_info 451

int get_proc_info(pid_t pid);
int get_system_info(void);

#endif