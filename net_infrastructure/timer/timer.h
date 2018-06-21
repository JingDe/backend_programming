/*
	通用的定时器实现
	仅包含：到期时间、回调函数
	接口：
	
*/
#ifndef TIMER_H_
#define TIMER_H_

#include<functional> // c++11
#include<time.h>

typedef std::function<void()> TimerCallback;

class Timer{
private:
	time_t due;// 到期的绝对时间，TODO:提高精度
	TimerCallback tcb;
	
public:
	Timer(time_t d, const TimerCallback& cb);
	
	~Timer();

	time_t getDue() const {
		return due;
	}

	void run() // TODO: thread safe??
	{
		tcb();
	}

};

#endif