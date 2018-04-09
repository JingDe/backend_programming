#include"Timer.h"

AtomicPointer Timer::s_numCreated_;


// time_t����������ʾEPOCH�������
time_t addTime(time_t ts, int seconds)
{
	return ts + seconds;
}

void Timer::restart(time_t now)
{
	if (repeat_)
	{
		expiration_ = now + interval_; // addTime(now, interval_);
	}
	else
	{
		expiration_ = 0;
	}
}