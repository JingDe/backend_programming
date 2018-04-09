#ifndef TIMER_H_
#define TIMER_H_

//#include"common/atomic_pointer.h"
#include"AtomicPointer_temp.h"

#include<ctime>
#include<functional>

class Timer {
public:
	typedef std::function<void(void)> TimerCallback;

	Timer(const TimerCallback& cb, time_t when, int interval)
		:callback_(cb),
		expiration_(when),
		interval_(interval),
		repeat_(interval>0),
		sequence_(reinterpret_cast<int>(s_numCreated_.incrementAndGet()))
	{
	}

	void run() const
	{
		callback_();
	}

	time_t expiration() const
	{
		return expiration_;
	}

	bool repeat() const
	{
		return repeat_;
	}

	int sequence() const
	{
		return sequence_;
	}

	void restart(time_t now);

	static int numCreated()
	{
		return reinterpret_cast<int>(s_numCreated_.Acquire_Load());
	}

private:
	time_t expiration_;
	TimerCallback callback_;
	const int interval_;
	const bool repeat_;
	const int sequence_;

	static AtomicPointer s_numCreated_;
};

#endif 

