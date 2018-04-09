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

	TimerId addTimer(const TimerCallback& cb, time_t when, int interval);

	void cancel(TimerId timerId);

private:
	void addTimerInLoop(Timer* timer);

	typedef std::pair<time_t, Timer*> Entry; // TODO: unique_ptr
	typedef std::set<Entry> TimerList;

	typedef std::pair<Timer*, int> ActiveTimer;
	typedef std::set<ActiveTimer> ActiveTimerSet;

	EventLoop * loop_;
	const int timerfd_;
	Channel timerfdChannel_;
	TimerList timers_; // 所有定时器

	std::vector<Timer*> expired_;

	ActiveTimerSet activeTimers_;
	bool callingExpiredTimers_;
	ActiveTimerSet cancelingTimers_;
};

#endif 