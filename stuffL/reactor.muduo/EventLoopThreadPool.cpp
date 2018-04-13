#include"EventLoopThreadPool.h"
#include"reactor.muduo/EventLoop.h"
#include"reactor.muduo/EventLoopThread.h"

#include<cassert>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
	:baseLoop_(baseLoop),
	started_(false),
	numThreads_(0),
	next_(0)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	assert(!started_);
	baseLoop_->assertInLoopThread(); // 在baseLoop_线程调用start()函数

	started_ = true;
	for (int i = 0; i < numThreads_; i++)
	{
		EventLoopThread* t = new EventLoopThread(cb);
		threads_.push_back(t);
		loops_.push_back(t->startLoop());
	}
	if (numThreads_ == 0 && cb)
	{
		cb(baseLoop_);
	}
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
	baseLoop_->assertInLoopThread();
	assert(started_);
	EventLoop* loop = baseLoop_;

	if (!loops_.empty())
	{
		loop = loops_[next_];
		++next_;
		if (static_cast<size_t>(next_) >= loops_.size())
			next_ = 0;
	}
	return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{

}