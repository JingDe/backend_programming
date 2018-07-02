
#include"timer.h"

Timer::Timer(time_t d, const TimerCallback& cb)
	:due(d), tcb(cb)
{}

Timer::~Timer()
{}
