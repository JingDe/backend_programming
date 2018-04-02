#include"EventLoop.h"
#include"logging.muduo/Logging.h"
#include"thread.muduo/CurrentThread.h"
#include"thread.muduo/thread_util.h"
#include"Channel.h"
#include"Poller.h"

#include<cassert>

#include<poll.h>

__thread EventLoop* t_loopInThisThread = 0;

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}
EventLoop::EventLoop():looping_(false),quit_(false),pid_(tid())
{
	if (t_loopInThisThread)
	{
		LOG_ERROR << "EventLoop already exits in this thread";
	}
	else
		t_loopInThisThread = this;
}

EventLoop::~EventLoop()
{
	assert(!looping_);
	t_loopInThisThread = 0;
}

void EventLoop::loop()
{
	assert(!looping_);
	AssertInLoopThread();
	looping_ = true;

	quit_ = false;
	LOG_INFO << "EventLoop " << this << " start looping";

	poll(NULL, 0, 5 * 1000); // ×èÈû5000ºÁÃë
	/*while (quit_ == false)
	{

	}*/
	LOG_INFO << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::quit()
{
	quit_ = true;

}

void EventLoop::AssertInLoopThread()
{
	pid_t callerId = tid(); // Îªcaller¼ÆËãpid_t
	if (callerId != pid_)
	{
		LOG_FATAL << "AssertInLoopThread failed: EventLoop "<<this<<" was created in "<<pid_<<" , current thread id = "<< callerId;
		return;
	}


}

void EventLoop::updateChannel(Channel* c)
{
	assert(c->ownerLoop() == this);
	AssertInLoopThread();
	poller_->updateChannel(c);
}