#include"Thread.h"
#include"logging.muduo/logging.h"

#include<string>
#include<cassert>
#include<functional>
#include<sys/types.h>
#include<unistd.h>

void* startThread(void* arg)
{
	ThreadData* td = static_cast<ThreadData*>(arg);
	/*ThreadFunc func = td->func_;
	func();*/
	td->runInThread();
	delete td;
	return NULL;
}


Thread::Thread(const ThreadFunc&func) :
	func_(func)
{
	// 计算tid_
}

void Thread::start()
{
	assert(started_ == false);
	// 创建线程执行func_函数 
	ThreadData *td=new ThreadData(func_, &tid_);
	if (pthread_create(&pthreadId_, NULL, startThread, td))
	{
		delete td;
		LOG_FATAL << "Failed in pthread_create"; // 线程创建失败，程序退出
	}
	else
	{
		started_ = true;
	}
}

int Thread::join()
{
	assert(started_);
	assert(!joined_);
	joined_ = true;
	return pthread_join(pthreadId_, NULL); // 线程join失败，返回结果
}