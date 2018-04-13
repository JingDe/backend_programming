#include"EventLoopThread.h"

#include<cassert>

#include"EventLoop.h"
#include"common/MutexLock.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
	:mutex_(),
	cond_(&mutex_),
	loop_(NULL),
	thread_(std::bind(&EventLoopThread::threadFunc, this)),
	exiting_(false),
	callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
	exiting_ = true;
	if (loop_ != NULL)
	{
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop()
{
	assert(!thread_.started());
	thread_.start();

	{
		MutexLock lock(mutex_);
		while (loop_ == NULL)
			cond_.Wait();
	}

	return loop_;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;

	if (callback_)
	{
		callback_(&loop);
	}
	{
		MutexLock lock(mutex_);
		loop_ = &loop;
		cond_.Signal();
	}
	loop.loop();
	// loopÍË³öÑ­»·
	loop_ = NULL;
}