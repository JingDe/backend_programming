#ifndef THREAD_UTIL_H_
#define THREAD_UTIL_H_

#include<stdint.h>
//#include<cstdint> // c++11  -std=c++11 or -std=gnu++11


#include<sys/types.h>

// ϵͳ���û���̵߳��ں˼�id
extern pid_t gettid();

// ��pthread_t����߳�id
extern uint64_t gettid2();

#endif