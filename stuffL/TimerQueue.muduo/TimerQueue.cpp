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

// �����ʱ����when��ʱ�䣬��timespec�ṹ���ʾ���롢���룩
//struct timespec howMuchTimeFromNow(time_t when)
//{
//	time_t now = time(NULL);
//	int64_t seconds = when - now;
//	struct timespec ts;
//	ts.tv_sec = seconds;
//	ts.tv_nsec = 0;
//	return ts;
//}

struct timespec howMuchTimeFromNow(Timestamp when)
{
	int64_t microseconds = when.microSecondsSinceEpoch()
		- Timestamp::now().microSecondsSinceEpoch();
	if (microseconds < 100)
		microseconds = 100;
	struct timespec ts;
	ts.tv_sec = static_cast<time_t>(microseconds / kMicroSecondsPerSecond);
	ts.tv_nsec = static_cast<long>((microseconds % kMicroSecondsPerSecond) * 1000);
	return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
	uint64_t howmany;
	ssize_t n = read(timerfd, &howmany, sizeof howmany);
	LOG_INFO << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
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
flags������0��ʾ��Զ�ʱ��������TFD_TIMER_ABSTIME��ʾ���Զ�ʱ��
*/

// ����timerfd��expireʱ��Ϊexpiration
void resetTimerfd(int timerfd, Timestamp expiration)
{
	LOG_DEBUG << "resetTimerfd: " << expiration.toString();

	struct itimerspec newValue;
	struct itimerspec oldValue;
	bzero(&newValue, sizeof newValue);
	bzero(&oldValue, sizeof oldValue);
	newValue.it_value = howMuchTimeFromNow(expiration); // �˿̾���expiration��timespecʱ��	
	
	int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
	if (ret)
		LOG_FATAL << "timerfd_settime()";
}


TimerQueue::TimerQueue(EventLoop* loop)
	:loop_(loop),
	timerfd_(createTimerfd()), // ����һ��timer����
	timerfdChannel_(loop, timerfd_), // ����һ��channel����עtimerfd_
	timers_(),
	expired_(), // Ĭ�Ϲ���һ����vector
	callingExpiredTimers_(false)
{
	// ����timerfdChannel_��read�ص�����
	// ��timerfdChannel_ע�ᵽloop��poller_
	timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this, std::placeholders::_1));
	timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
	timerfdChannel_.disableAll();
	timerfdChannel_.remove();
	close(timerfd_);

	for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
		delete it->second; 
}


TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, double interval)
{
	Timer* timer = new Timer(cb, when, interval);
	//TimerPtr timer(new Timer(cb, when, interval));
	loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	loop_->assertInLoopThread();
	bool earliestChanged = insert(timer);

	if (earliestChanged) // �����һ������expire��Timer������timerfd��expireʱ��
		resetTimerfd(timerfd_, timer->expiration());
}

// ��timers_��activeTimers_�в���timer�������Ƿ���뵽��ͷ
bool TimerQueue::insert(Timer* timer)
{
	loop_->assertInLoopThread();
	assert(timers_.size() == activeTimers_.size());
	bool earliestChanged = false;
	Timestamp when = timer->expiration();
	TimerList::iterator it = timers_.begin();
	if (it == timers_.end() || when < it->first)
	{
		earliestChanged = true;
	}
	{
		//TimerPtr timerP(timer);
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

// ͬʱ��activeTimers_��timers_��ɾ�������뵽cancelingTimers_��
void TimerQueue::cancelInLoop(TimerId timerId)
{
	loop_->assertInLoopThread();
	assert(timers_.size() == activeTimers_.size());
	ActiveTimer timer(timerId.timer_, timerId.sequence_);
	ActiveTimerSet::iterator it = activeTimers_.find(timer); // Ѱ��ActiveTimer
	if (it != activeTimers_.end()) // cancelδ���ڵ�timer
	{
		size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
		assert(n == 1);
		delete it->first;
		activeTimers_.erase(it);
	}
	// cancel���ڵ�timer����cancel/��ע��
	else if (callingExpiredTimers_) // ���ڵ��õ���timer�ص��ڼ䣬��¼����Щcancel����
		cancelingTimers_.insert(timer); 
	assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::getExpired(Timestamp now)
{
	assert(timers_.size() == activeTimers_.size());
	expired_.clear();
	Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
	TimerList::iterator end = timers_.lower_bound(sentry); // ���Ҵ��ڵ���sentry�ĵ�������endָ���һ��δ���ڵĶ�ʱ��
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
}

void TimerQueue::handleRead(Timestamp pollReturnTime)
{	
	loop_->assertInLoopThread();
	Timestamp now(Timestamp::now());
	LOG_DEBUG << "pollReturnTime: " << pollReturnTime.toString() << ", now: " << now.toString();

	readTimerfd(timerfd_, now); // �ں���timerfd_��д

	//getExpired(now);
	getExpired(pollReturnTime); // ������й���timer

	callingExpiredTimers_ = true;
	cancelingTimers_.clear(); // ���ڵ������е���timer�ڼ䣬��¼����Щcancel����
	
	// ���ù���timer�Ļص�����,�п�����ע�������ص���cancel��ǰtimer
	for (std::vector<Entry>::iterator it = expired_.begin(); it != expired_.end(); ++it) 
		it->second->run(); 
	callingExpiredTimers_ = false;

	reset(now); // ���������Ե�timer��ɾ������timer������timerfd�Ĺ���ʱ��
}

void TimerQueue::reset(Timestamp now)
{
	Timestamp nextExpire;

	for (std::vector<Entry>::const_iterator it = expired_.begin(); it != expired_.end(); ++it)
	{
		ActiveTimer timer(it->second, it->second->sequence());
		if (it->second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) // ������cancel��timer��ӻ�timers_��activeTimers_��
		{
			it->second->restart(now);
			insert(it->second); // ���������Ե�timer
		}
		else
			delete it->second;
	}

	if (!timers_.empty())
		nextExpire = timers_.begin()->second->expiration();
	
	if(nextExpire.valid())
		resetTimerfd(timerfd_, nextExpire); // ����timerfd��expireʱ��Ϊ��һ��timer��expireʱ��
}