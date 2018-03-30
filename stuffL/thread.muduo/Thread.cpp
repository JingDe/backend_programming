#include"Thread.h"
#include"logging.muduo/Logging.h"
#include"thread.muduo/CurrentThread.h"

#include<string>
#include<cassert>
#include<functional>
#include<sys/types.h>
#include<unistd.h>

#include <pthread.h>

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
	func_(func), started_(false), joined_(false)
{
	// 计算tid_
}

Thread::~Thread()
{
	if (started_ && !joined_)
		pthread_detach(pthreadId_); // detach线程：当detach线程终止时，资源自动释放给系统，不需要另一个线程join它
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

void ThreadData::runInThread()
{
	*tid_ = tid();
	tid_ = NULL;

	try {
		func_();
	}
	catch (const std::exception& ex)
	{
		fprintf(stderr, "reason: %s\n", ex.what());
		abort();
	}
	catch (...)
	{
		throw;
	}
}