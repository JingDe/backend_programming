#include"Timer_TS.h"


AtomicPointer Timer::s_numCreated_;


void Timer::restart(Timestamp now) // time_t «long int¿‡–Õ
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