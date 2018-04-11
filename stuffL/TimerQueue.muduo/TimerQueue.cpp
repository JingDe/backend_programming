#include"TimerQueue.h"
#include"reactor.muduo/EventLoop.h"
#include"TimerId.h"
#include"Timer.h"

//#include<iostream>
#include<cassert>
#include<cstdio>

#include<unistd.h>
#include<sys/timerfd.h>

#include"logging.muduo/Logging.h"

static const int kMicroSecondsPerSecond = 1000 * 1000;

int createTimerfd()
{
	int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (timerfd < 0)
		LOG_FATAL << "failed int timerfd_create";
	return timerfd;
}

struct timespec howMuchTimeFromNow(time_t when)
{
	time_t now = time(NULL);
	int64_t microseconds = when - now;
	if (microseconds < 100)
		microseconds = 100;
	struct timespec ts;
	ts.tv_sec = static_cast<time_t>(microseconds/kMicroSecondsPerSecond);
	ts.tv_nsec = static_cast<long>((microseconds % kMicroSecondsPerSecond)*1000);
	return ts;
}

void readTimerfd(int timerfd, time_t now)
{
	uint64_t howmany;
	ssize_t n = read(timerfd, &howmany, sizeof howmany);
	LOG_INFO << "TimerQueue::handleRead() " << howmany << " at " << now;
	if (n != sizeof howmany)
		LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
}

/*
struct itimerspec{
	struct timespec it_iterval;
	struct timespec it_value;
};

struct timespec{
	time_t tv_sec;
	long tv_nsec;
};

int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
flags：等于0表示相对定时器，等于TFD_TIMER_ABSTIME表示绝对定时器
*/

void resetTimerfd(int timerfd, time_t expiration)
{
	LOG_DEBUG << "resetTimerfd: " << expiration;

	struct itimerspec newValue;
	struct itimerspec oldValue;
	bzero(&newValue, sizeof newValue);
	bzero(&oldValue, sizeof oldValue);
	newValue.it_value = howMuchTimeFromNow(expiration); // 此刻距离expiration的timespec时间	
	int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
	if (ret)
		LOG_FATAL << "timerfd_settime()";
}


TimerQueue::TimerQueue(EventLoop* loop)
	:loop_(loop),
	timerfd_(createTimerfd()), // 创建一个timer对象
	timerfdChannel_(loop, timerfd_), // 创建一个channel，关注timerfd_
	timers_(),
	expired_(), // 默认构造一个空vector
	callingExpiredTimers_(false)
{
	// 设置timerfdChannel_的read回调函数
	// 将timerfdChannel_注册到loop的poller_
	timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this, std::placeholders::_1));
	timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
	timerfdChannel_.disableAll();
	timerfdChannel_.remove();
	close(timerfd_);

	for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
		delete it->second; // TODO : unique_ptr
}


TimerId TimerQueue::addTimer(const TimerCallback& cb, time_t when, long interval)
{
	Timer* timer = new Timer(cb, when, interval);
	// TimerPtr timer(new Timer());
	loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	loop_->assertInLoopThread();
	bool earliestChanged = insert(timer);

	if (earliestChanged) // 当添加一个更早expire的Timer，更新timerfd的expire时间
		resetTimerfd(timerfd_, timer->expiration());
}

// 在timers_和activeTimers_中插入timer，返回是否插入到开头
bool TimerQueue::insert(Timer* timer)
{
	loop_->assertInLoopThread();
	assert(timers_.size() == activeTimers_.size());
	bool earliestChanged = false;
	time_t when = timer->expiration();
	TimerList::iterator it = timers_.begin();
	if (it == timers_.end() || when < it->first)
	{
		earliestChanged = true;
	}
	{
		std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer)); 
			// std::pair<iterator, bool> insert(const value_type& value);
		assert(result.second);
	}
	{
		std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
		assert(result.second);
	}
	assert(timers_.size() == activeTimers_.size());
	return earliestChanged;
}

void TimerQueue::cancel(TimerId timerId)
{
	loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

/*
ActiveTimer:       <Timer*, sequence>
	TimerId:           Timer*, sequence
TimerList::Entry:  <time_t, Timer*> 
*/
// 同时从activeTimers_和timers_中删除，插入到cancelingTimers_中
void TimerQueue::cancelInLoop(TimerId timerId)
{
	loop_->assertInLoopThread();
	assert(timers_.size() == activeTimers_.size());
	ActiveTimer timer(timerId.timer_, timerId.sequence_);
	ActiveTimerSet::iterator it = activeTimers_.find(timer); // 寻找ActiveTimer
	if (it != activeTimers_.end())
	{
		size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
		assert(n == 1);
		delete it->first;
		activeTimers_.erase(it);
	}
	else if (callingExpiredTimers_)
		cancelingTimers_.insert(timer);
	assert(timers_.size() == activeTimers_.size());
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(time_t now)
{
	// 打印timers列表
	/*for (TimerList::const_iterator cit = timers_.begin(); cit != timers_.end(); cit++)
	{
		std::cout << cit->first<<", ";
	}
	std::cout << "now : "<<now << std::endl;*/

	assert(timers_.size() == activeTimers_.size());
	expired_.clear();
	Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
	TimerList::iterator end = timers_.lower_bound(sentry); // 查找大于等于sentry的迭代器，end指向第一个未到期的定时器
	assert(end == timers_.end() || now < end->first);
	std::copy(timers_.begin(), end, back_inserter(expired_));

	LOG_DEBUG << timers_.size() << " timers, " << expired_.size() << " expired timers";

	// template<class Container> std::back_inserter_iterator<Container> back_inserter(Container& c);
	timers_.erase(timers_.begin(), end);

	for (std::vector<Entry>::iterator it = expired_.begin(); it != expired_.end(); it++)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		size_t n = activeTimers_.erase(timer);
		assert(n == 1);
	}
	assert(timers_.size() == activeTimers_.size());
	return expired_;
}

void TimerQueue::handleRead(time_t pollReturnTime)
{	
	loop_->assertInLoopThread();
	time_t now = time(NULL);
	LOG_DEBUG << "pollReturnTime: " << pollReturnTime << ", now: " << now;

	readTimerfd(timerfd_, now); // 内核往timerfd_里写

	//getExpired(now);
	getExpired(pollReturnTime);

	callingExpiredTimers_ = true;
	cancelingTimers_.clear();
	
	for (std::vector<Entry>::iterator it = expired_.begin(); it != expired_.end(); ++it)
		it->second->run();
	callingExpiredTimers_ = false;

	reset(now);
}

void TimerQueue::reset(time_t now)
{
	time_t nextExpire;

	for (std::vector<Entry>::const_iterator it = expired_.begin(); it != expired_.end(); ++it)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		if (it->second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
		{
			it->second->restart(now);
			insert(it->second); // 插入周期性的timer
		}
		else
			delete it->second;
	}

	if (!timers_.empty())
		nextExpire = timers_.begin()->second->expiration();
	
	if(nextExpire>now)
		resetTimerfd(timerfd_, nextExpire); // 设置timerfd的expire时间为第一个timer的expire时间
}