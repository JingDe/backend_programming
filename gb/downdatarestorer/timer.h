#ifndef TIMER_H_
#define TIMER_H_


#include "owlog.h"
#include "threadpool.h"
#include<exception>

class Timer : public ThreadPool {
public:
	Timer();
	virtual ~Timer();

	/**
	 * start timer after delay time
	 * @param intervalSeconds: repeat run per interval by second
	 * @param delay: delay x ms to start
	 * @param repeatTime: repeat how many times, -1 means repeat forever
	 */
	void start(int32_t intervalSeconds, int32_t delay = 0, int32_t repeatTime = -1);

	void cancel();

	virtual void svc(void);
	virtual bool resetIntervalTime(int32_t intervalSeconds);

	virtual void run(void){
		throw std::exception();
	}

private:
	bool m_isStarted;
	int32_t m_repeatTime;
	int32_t m_repeatCount;
	int32_t m_delay;
	int32_t m_intervalSeconds;
	OWLog m_logger;
};

#endif /*TIMER_H_*/
