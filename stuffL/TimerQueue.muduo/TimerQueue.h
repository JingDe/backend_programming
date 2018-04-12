#ifndef TIMERQUEUE_H_
#define TIMERQUEUE_H_

#include"reactor.muduo/Channel.h"

#include<set>
#include<vector>


class EventLoop;
class TimerId;
class Timer;

class TimerQueue {
public:
	typedef std::function<void(void)> TimerCallback;

	explicit TimerQueue(EventLoop* loop);
	~TimerQueue();

	TimerId addTimer(const TimerCallback& cb, time_t when, long interval);

	void cancel(TimerId timerId);

	int timerfd() const { return timerfd_;  }

private:
	typedef std::unique_ptr<Timer> TimerPtr;
	typedef std::pair<time_t, TimerPtr> Entry; 
	typedef std::set<Entry> TimerList;

	typedef std::pair<TimerPtr, int> ActiveTimer; // sequence
	typedef std::set<ActiveTimer> ActiveTimerSet;

	void addTimerInLoop(Timer* timer);
	void cancelInLoop(TimerId timer);
	bool insert(Timer*);
	std::vector<TimerQueue::Entry> getExpired(time_t now);
	void handleRead(time_t pollReturnTime);
	//void reset(const std::vector<Entry>& expired, time_t now);
	void reset(time_t now);

	EventLoop * loop_;
	const int timerfd_;
	Channel timerfdChannel_;
	TimerList timers_; // 所有定时器

	bool callingExpiredTimers_;
	std::vector<Entry> expired_;

	ActiveTimerSet activeTimers_; // activeTimers_和TimerList的Timer*相同
	
	ActiveTimerSet cancelingTimers_; // 取消了的Timer
};

#endif 