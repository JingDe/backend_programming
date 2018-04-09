#ifndef TIMERID_H_
#define TIMERID_H_

class Timer;

class TimerId {
public:
	TimerId() :timer_(NULL), sequence_(0) {}

	TimerId(Timer* timer, int seq) :timer_(timer), sequence_(seq) {}

	friend class TimerQueue;

private:
	Timer * timer_;
	int sequence_;
};

#endif