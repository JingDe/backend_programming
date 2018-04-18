#include"Timer.h"

AtomicPointer Timer::s_numCreated_;


// time_t����������ʾEPOCH�������, time_t��long int����
//time_t addTime(time_t ts, int seconds)
//{
//	return ts + seconds;
//}

void Timer::restart(Timestamp now)
{
	if (repeat_)
	{
		expiration_ = addTime(now, interval_);
	}
	else
	{
		expiration_ = Timestamp::invalid();
	}
}