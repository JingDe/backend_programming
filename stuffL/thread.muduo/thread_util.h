#ifndef THREAD_UTIL_H_
#define THREAD_UTIL_H_

#include<stdint.h>
//#include<cstdint> // c++11  -std=c++11 or -std=gnu++11


#include<sys/types.h>

// 系统调用获得线程的内核级id
extern pid_t gettid();

// 从pthread_t获得线程id
extern uint64_t gettid2();

#endif