#ifndef __THREAD_TEST_H_
#define __THREAD_TEST_H_

#include<sys/types.h>


class ThreadObject {
public:
	ThreadObject();

private:
	pid_t threadID_;
};

// 全局变量在头文件中extern声明

// __thread变量呢？
__thread ThreadObject* t_objectInThread = 0;

#endif