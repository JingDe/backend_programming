#include"TimerQueue.h"

#include"logging.muduo/Logging.h"
#include"reactor.muduo/EventLoop.h"
#include"TimerId.h"
#include"Timer.h"

#include<unistd.h>
#include<cstdio>
#include<sys/timerfd.h>

int createTimerfd()
{
	int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (timerfd < 0)
		LOG_FATAL << "failed int timerfd_create";
	return timerfd;
}

TimerQueue::TimerQueue(EventLoop* loop)
	:loop_(loop),
	timerfd_(createTimerfd()), // 创建一个timer对象
	timerfdChannel_(loop, timerfd_), // 创建一个channel，关注timerfd_
	timers_(),
	callingExpiredTimers_(false),
	expired_()
{}

TimerQueue::~TimerQueue()
{
	timerfdChannel_.disableAll();
	timerfdChannel_.remove();
	close(timerfd_);

	for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
		delete it->second; // TODO : unique_ptr
}

TimerId TimerQueue::addTimer(const TimerCallback& cb, time_t when, int interval)
{
	Timer* timer = new Timer(cb, when, interval);
	// TimerPtr timer(new Timer());
	loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer)
{

}