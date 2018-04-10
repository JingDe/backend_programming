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
	typedef std::pair<time_t, Timer*> Entry; // TODO: unique_ptr
	typedef std::set<Entry> TimerList;

	typedef std::pair<Timer*, int> ActiveTimer; // <Timer*, sequence>
	typedef std::set<ActiveTimer> ActiveTimerSet;

	void addTimerInLoop(Timer* timer);
	void cancelInLoop(TimerId timer);
	bool insert(Timer*);
	std::vector<TimerQueue::Entry> getExpired(time_t now);

	EventLoop * loop_;
	const int timerfd_;
	Channel timerfdChannel_;
	TimerList timers_; // ���ж�ʱ��

	std::vector<Entry> expired_;

	ActiveTimerSet activeTimers_; // activeTimers_��TimerList��Timer*��ͬ
	bool callingExpiredTimers_;
	ActiveTimerSet cancelingTimers_; // ȡ���˵�Timer
};

#endif 