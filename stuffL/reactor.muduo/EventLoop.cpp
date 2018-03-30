#include"EventLoop.h"
#include"logging.muduo\Logging.h"

#include<cassert>

__thread EventLoop* t_loopInThisThread = 0;


EventLoop::EventLoop():looping_(false),pid_()
{
	if (t_loopInThisThread)
	{
		LOG_INFO << "EventLoop already exits in this thread";
	}
	else
		t_loopInThisThread = this;
}

EventLoop::EventLoop()
{
	assert(!looping_);
	t_loopInThisThread = 0;
}