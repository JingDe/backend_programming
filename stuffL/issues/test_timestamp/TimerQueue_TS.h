#ifndef TIMERQUEUE_H_
#define TIMERQUEUE_H_

#include"Channel_TS.h"
#include"TimerQueue.muduo/Timestamp.h"

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

	TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

	void cancel(TimerId timerId);

	int timerfd() const { return timerfd_;  }

private:
	typedef std::pair<Timestamp, Timer*> Entry; // TODO: unique_ptr
	typedef std::set<Entry> TimerList;

	typedef std::pair<Timer*, int> ActiveTimer; // <Timer*, sequence>
	typedef std::set<ActiveTimer> ActiveTimerSet;

	void addTimerInLoop(Timer* timer);
	void cancelInLoop(TimerId timer);
	bool insert(Timer*);
	std::vector<TimerQueue::Entry> getExpired(Timestamp now);
	void handleRead(Timestamp pollReturnTime);
	//void reset(const std::vector<Entry>& expired, time_t now);
	void reset(Timestamp now);

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