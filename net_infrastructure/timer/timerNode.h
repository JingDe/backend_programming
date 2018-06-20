
#ifndef TIMER_NODE_H_
#define TIMER_NODE_H_

#include<time.h>
#include<functional>

typedef std::function<void()> TimerCallback;

struct Timer{
	time_t due;
	TimerCallback tcb;

	Timer* prev;
	Timer* next;
	
	Timer(time_t t, const TimerCallback &cb)
	{
		due=t;
		tcb=cb;
	}
};

#endif