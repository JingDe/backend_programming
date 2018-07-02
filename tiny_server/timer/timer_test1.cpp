
//（1）设置epoll_wait的timeout，timeout时处理超时定时器

/*
	g++ timer_test1.cpp timertree.cpp timer.cpp -llogging -L../../lib -I../../ -o timer_test1.out -std=c++11

*/

#include"logging.muduo/Logging.h"
#include"timertree.h"

#include<sys/epoll.h>
#include<unistd.h>

const static int EPEVENTS=10;
const static int TOUT=5*1000;//默认超时5秒


void timerRoutine()
{
	LOG_INFO<<"timer expired";
}

int main()
{
	Logger::setLogLevel(Logger::TRACE);

	int epfd;
	struct epoll_event events[EPEVENTS];
	TimerTree tt;

	epfd=epoll_create(10);
	if(epfd<0)
	{
		LOG_FATAL<<"epoll_create failed";
		return 0;
	}

	time_t now=time(NULL);
	Timer t1(now+5, timerRoutine);
	Timer t2(now+10, timerRoutine);
	Timer t3(now+15, timerRoutine);
	tt.addTimer(&t1);
	tt.addTimer(&t2);
	tt.addTimer(&t3);
	tt.debugPrint();

	while(true)
	{
		// 获得最近的定时器到期时间
		now=time(NULL);
		int timeout=tt.getMinExpired()-now; // 毫秒单位
		if(timeout<0)
			break;
		int nevt=epoll_wait(epfd, events, EPEVENTS, timeout);
		if(nevt<0)
		{
			LOG_FATAL<<"epoll_wait failed";
			break;
		}


		if(nevt==0)
		{
			LOG_INFO<<"timeout";
			// 处理超时定时器
			tt.handleExpired(now+timeout);
		}
		// epoll_wait之后不进行处理？？

	}
	LOG_DEBUG<<"break while";
	tt.handleExpired(now);

	close(epfd);

	return 0;
}
