

// （2）使用timerfd通知epoll_wait，统一事件源（同IO、信号事件）

/*
	g++ timer_test2.cpp timertree.cpp timer.cpp -llogging -L../../lib -I../../ -o timer_test2.out -std=c++11
*/

#include"logging.muduo/Logging.h"
#include"timertree.h"


#include<unistd.h>
#include<sys/timerfd.h>
#include<sys/epoll.h>

class TimerWrap
{
private:
	int timerfd;
	TimerTree tt;

	int CreateTimerfd()
	{
		int fd=timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
		if(fd<0)
			LOG_FATAL<<"timerfd_create failed "<<strerror(errno);
		return fd;
	}
	// 获得绝对时间t距离此刻的时间，用timespec表示，秒、纳秒
	struct timespec howMuchTimeFromNow(time_t t)
	{
		time_t now=time(NULL);
		struct timespec result;
		result.tv_sec=t-now;
		result.tv_nsec=0;
		return result;
	}

public:
	TimerWrap():timerfd(CreateTimerfd()), tt(){

	}
	~TimerWrap()
	{
		if(timerfd>0)
			close(timerfd);
	}

	void addTimer(Timer* t)
	{
		tt.addTimer(t);
		resetTimerfd();
	}
	void delTimer(Timer* t)
	{
		tt.delTimer(t);
		resetTimerfd();
	}

	int getTimerfd(){
		return timerfd;
	}

	// FIXME 设置绝对时间没起作用？？

	void resetTimerfd()//调用timerfd_settime重设定时器的通知时间
	{
		time_t sec=tt.getMinExpired();
		struct itimerspec tspec;
		//tspec.it_interval.tv_sec=0;// 不重复
		//tspec.it_interval.tv_nsec=0;
		bzero(&tspec, sizeof tspec);
		tspec.it_value.tv_sec=sec;// 由flags决定含义是相对时间还是绝对时间
		tspec.it_value.tv_nsec=0;
		int ret=timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &tspec, NULL);
		// flags，0表示相对时间，TFD_TIMER_ABSTIME绝对时间
		if(ret<0)
			LOG_FATAL<<"timerfd_settime failed "<<strerror(errno);
	}


	void resetTimerfd1()
	{
		time_t sec=tt.getMinExpired();
		if(sec<0)
			return;
		struct itimerspec tspec;
		bzero(&tspec, sizeof tspec);
		tspec.it_value=howMuchTimeFromNow(sec);
		int ret=timerfd_settime(timerfd, 0, &tspec, NULL);
		if(ret<0)
			LOG_FATAL<<"timerfd_settime failed "<<strerror(errno);
	}
	/*
	void readTimerfd()
	{
		char buf[10];
		int n=read(timerfd, buf, sizeof buf);
		if(n<0)
			LOG_FATAL<<"read timerfd failed";
		LOG_DEBUG<<"read "<<n<<" bytes";
		buf[n]=0;
		LOG_DEBUG<<"read : "<<buf;
	}
	*/
	void readTimerfd()
	{
		uint64_t data;
		int n=read(timerfd, &data, sizeof data);
		if(n!=sizeof data)
			LOG_ERROR<<"read timerfd failed";
		LOG_INFO<<"read "<<n<<" bytes : "<<data;//读到了1
	}

	void handleExpired(time_t now)
	{
		readTimerfd();
		tt.handleExpired(now);
		resetTimerfd();
	}
	void debugPrint()
	{
		tt.debugPrint();
	}
};

const static int TOUT=5*1000; // 毫秒单位
const static int EPEVENTS=10;

void addfd(int epfd, int fd, uint32_t events)
{
	struct epoll_event evt;
	evt.events=events;
	evt.data.fd=fd;
	int ret=epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evt);
	if(ret<0)
		LOG_FATAL<<"epoll_ctl ADD failed "<<strerror(errno);
}

void timerRoutine()
{
	LOG_INFO<<"timer expired";
}

int main()
{
	Logger::setLogLevel(Logger::DEBUG);

	int epfd;
	struct epoll_event events[EPEVENTS];
	int timerfd;

	epfd=epoll_create(EPEVENTS);
	if(epfd<0)
	{
		LOG_FATAL<<"epoll_create failed "<<strerror(errno);
		return 0;
	}

	TimerWrap tw;
	time_t now=time(NULL);
	Timer t1(now+25, timerRoutine);
	Timer t2(now+30, timerRoutine);
	Timer t3(now+15, timerRoutine);
	tw.addTimer(&t1);
	tw.addTimer(&t2);
	tw.addTimer(&t3);
	tw.debugPrint();

	timerfd=tw.getTimerfd();
	LOG_DEBUG<<"timerfd = "<<timerfd;
	addfd(epfd, timerfd, EPOLLIN);

	while(true)
	{
		int nevt=epoll_wait(epfd, events, EPEVENTS, -1);
		now=time(NULL);
		if(nevt<0)
		{
			LOG_FATAL<<"epoll_wait failed "<<strerror(errno);
			break;
		}

		for(int i=0; i<nevt; i++)
		{
			uint32_t evts=events[i].events;
			int fd=events[i].data.fd;

			if(fd==timerfd)
			{
				// 处理超时事件
				tw.handleExpired(now);
			}
			else
			{
				LOG_INFO<<"someting else happened";
			}
		}
	}

	close(epfd);

	return 0;
}
