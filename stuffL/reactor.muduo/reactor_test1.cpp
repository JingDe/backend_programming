#include"Channel.h"
#include"EventLoop.h"

#include<cstdio>
#include<sys/timerfd.h>

#include<unistd.h>
#include<strings.h>

EventLoop* g_loop;


void timeout(time_t t)
{
	printf("timeout\n");
	g_loop->quit();
}

int main()
{
	EventLoop loop;
	g_loop = &loop;

	int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	// 创建一个timer对象，通过文件描述符传递超时通知
	// CLOCK_MONOTONIC 相对时间，不受系统时间修改的影响
	// TFD_NONBLOCK设置timerfd的O_NONBLOCK标志，该标志使read/write等出错而不是阻塞
	// TFD_CLOEXEC设置timerfd的O_CLOEXEC标志，该标志使execve之后文件关闭

	Channel channel(&loop, timerfd);
	channel.setReadCallback(timeout);
	channel.enableReading();

	struct itimerspec howlong;
	bzero(&howlong, sizeof howlong);
	howlong.it_value.tv_sec = 5;
	timerfd_settime(timerfd, 0, &howlong, NULL);
		// 开始timer
	/*
	struct itimerspec{
		struct timespec it_interval; // 周期性timer的间隔
		struct timespec it_value; // 初始expiration时间
	};

	struct timespec{
		time_t tv_sec; 秒
		long tv_nsec; 纳秒
	};
	*/

	loop.loop();

	close(timerfd);

	return 0;
}