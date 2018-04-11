#include<iostream>

#include<unistd.h>
#include<sys/timerfd.h>

#include"logging.muduo/Logging.h"

namespace timerfd_test {


	static const int kMicroSecondsPerSecond = 1000 * 1000;

	int createTimerfd()
	{
		int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
		if (timerfd < 0)
			LOG_FATAL << "failed int timerfd_create";
		return timerfd;
	}

	struct timespec howMuchTimeFromNow(time_t when)
	{
		time_t now = time(NULL);
		int64_t microseconds = when - now;
		if (microseconds < 100)
			microseconds = 100;
		struct timespec ts;
		ts.tv_sec = static_cast<time_t>(microseconds / kMicroSecondsPerSecond);
		ts.tv_nsec = static_cast<long>((microseconds % kMicroSecondsPerSecond) * 1000);
		return ts;
	}

	void resetTimerfd(int timerfd, time_t expiration)
	{
		struct itimerspec newValue;
		struct itimerspec oldValue;
		bzero(&newValue, sizeof newValue);
		bzero(&oldValue, sizeof oldValue);
		newValue.it_value = howMuchTimeFromNow(expiration); // 此刻距离expiration的timespec时间
		int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
		if (ret)
			LOG_FATAL << "timerfd_settime()";
	}

	inline time_t addTime(time_t t, long seconds)
	{
		int64_t delta = static_cast<int64_t>(seconds * kMicroSecondsPerSecond); // int64_t = long long
		time_t result = t + delta;
		return result;
	}

	void readTimerfd(int timerfd, time_t now)
	{
		uint64_t howmany;
		ssize_t n = read(timerfd, &howmany, sizeof howmany);
		LOG_INFO << "TimerQueue::readTimerfd() " << howmany << " at " << now;
		if (n != sizeof howmany)
			LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
	}
	
void test()
{
	
	int timerfd = timerfd_test::createTimerfd();
	time_t now = time(NULL);
	timerfd_test::resetTimerfd(timerfd, timerfd_test::addTime(now, 10));

	sleep(15); // 等待timerfd expire


	now = time(NULL);
	timerfd_test::readTimerfd(timerfd, now);


}
}
