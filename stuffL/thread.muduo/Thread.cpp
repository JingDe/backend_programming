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
	// ����tid_
}

Thread::~Thread()
{
	if (started_ && !joined_)
		pthread_detach(pthreadId_); // detach�̣߳���detach�߳���ֹʱ����Դ�Զ��ͷŸ�ϵͳ������Ҫ��һ���߳�join��
}

void Thread::start()
{
	assert(started_ == false);
	// �����߳�ִ��func_���� 
	ThreadData *td=new ThreadData(func_, &tid_);
	if (pthread_create(&pthreadId_, NULL, startThread, td))
	{
		delete td;
		LOG_FATAL << "Failed in pthread_create"; // �̴߳���ʧ�ܣ������˳�
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
	return pthread_join(pthreadId_, NULL); // �߳�joinʧ�ܣ����ؽ��
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