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
	// ����һ��timer����ͨ���ļ����������ݳ�ʱ֪ͨ
	// CLOCK_MONOTONIC ���ʱ�䣬����ϵͳʱ���޸ĵ�Ӱ��
	// TFD_NONBLOCK����timerfd��O_NONBLOCK��־���ñ�־ʹread/write�ȳ������������
	// TFD_CLOEXEC����timerfd��O_CLOEXEC��־���ñ�־ʹexecve֮���ļ��ر�

	Channel channel(&loop, timerfd);
	channel.setReadCallback(timeout);
	channel.enableReading();

	struct itimerspec howlong;
	bzero(&howlong, sizeof howlong);
	howlong.it_value.tv_sec = 5;
	timerfd_settime(timerfd, 0, &howlong, NULL);
		// ��ʼtimer
	/*
	struct itimerspec{
		struct timespec it_interval; // ������timer�ļ��
		struct timespec it_value; // ��ʼexpirationʱ��
	};

	struct timespec{
		time_t tv_sec; ��
		long tv_nsec; ����
	};
	*/

	loop.loop();

	close(timerfd);

	return 0;
}