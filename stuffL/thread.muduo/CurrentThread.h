#ifndef CURRENT_THREAD_H_
#define CURRENT_THREAD_H_

#include<sys/types.h>
#include"thread_util.h"


extern pid_t t_cachedPid;


extern void cacheTid();
extern pid_t tid();

#endif