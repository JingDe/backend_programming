#ifndef TIMER_H_
#define TIMER_H_

//#include"common/atomic_pointer.h"
#include"AtomicPointer_temp.h"

#include<ctime>
#include<functional>

class Timer {
public:
	typedef std::function<void(void)> TimerCallback;

	Timer(const TimerCallback& cb, time_t when, long interval)
		:callback_(cb),
		expiration_(when),
		interval_(interval),
		repeat_(interval>0),
		sequence_(reinterpret_cast<long>(s_numCreated_.incrementAndGet()))
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

	long sequence() const
	{
		return sequence_;
	}

	void restart(time_t now);

	static int numCreated()
	{
		return reinterpret_cast<intptr_t>(s_numCreated_.Acquire_Load());
	}

private:
	TimerCallback callback_;
	time_t expiration_;
	const long interval_;
	const bool repeat_;
	const long sequence_;

	static AtomicPointer s_numCreated_; // 创建的定时器的数目，序号sequence
};

#endif 

