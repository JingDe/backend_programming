#include"EventLoop.h"
#include"Channel.h"
#include"Poller.h"
#include"TimerQueue.muduo/TimerId.h"

#include<cassert>

#include<poll.h>

#include"logging.muduo/Logging.h"
#include"thread.muduo/CurrentThread.h"
#include"thread.muduo/thread_util.h"

__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000; // poll����ʱ��10��
static const int kMicroSecondsPerSecond = 1000 * 1000;

inline time_t addTime(time_t t, long int seconds)
{
	int64_t delta = static_cast<int64_t>(seconds * kMicroSecondsPerSecond); // long long

}


EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

EventLoop::EventLoop()
	:looping_(false),quit_(false),pid_(tid()),
	poller_(Poller::newDefaultPoller(this)),
	currentActiveChannel_(NULL)
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
	assertInLoopThread();
	looping_ = true;

	quit_ = false;
	LOG_INFO << "EventLoop " << this << " start looping";

	//poll(NULL, 0, 5 * 1000); // ����5000����
	while (quit_ == false)
	{
		activeChannels_.clear();
		pollReturnTime_=poller_->poll(kPollTimeMs, &activeChannels_);
		if (Logger::logLevel() <= Logger::INFO)
			printActiveChannels();

		for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++)
		{
			currentActiveChannel_ = *it;
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;
		doPendingFunctors();
	}
	LOG_INFO << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::quit()
{
	quit_ = true;

}

void EventLoop::assertInLoopThread()
{
	pid_t callerId = tid(); // Ϊcaller����pid_t
	if (callerId != pid_)
	{
		LOG_FATAL << "AssertInLoopThread failed: EventLoop "<<this<<" was created in "<<pid_<<" , current thread id = "<< callerId;
		return;
	}
	
}

void EventLoop::updateChannel(Channel* c)
{
	assert(c->ownerLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(c);
}

void EventLoop::removeChannel(Channel* c)
{
	assert(c->ownerLoop() == this);
	assertInLoopThread();
	poller_->removeChannel(c);
}

void EventLoop::printActiveChannels() const 
{

}

void EventLoop::doPendingFunctors()
{

}

TimerId EventLoop::runAfter(long delay, const TimerCallback& cb)
{
	time_t now = time(NULL);
	time_t time(addTime(now, delay));
	return runAt(time, cb);
}