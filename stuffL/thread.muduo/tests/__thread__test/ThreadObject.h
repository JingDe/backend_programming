#ifndef __THREAD_TEST_H_
#define __THREAD_TEST_H_

#include<sys/types.h>


class ThreadObject {
public:
	ThreadObject();

private:
	pid_t threadID_;
};

// ȫ�ֱ�����ͷ�ļ���extern����

// __thread�����أ�
__thread ThreadObject* t_objectInThread = 0;

#endif