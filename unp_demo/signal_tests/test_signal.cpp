/*
	信号，与进程、线程测试
*/
#include<stdio.h>
#include<iostream>
#include<cstdint>
#include<cstring>
#include<signal.h>
#include<sys/syscall.h>
#include<errno.h>
#include<strings.h>

#define GET_TID syscall(__NR_gettid)

pid_t gettid()
{
    return static_cast<pid_t>(syscall(SYS_gettid));
}

#define LOG(format, ...) printf("%lld, "format, static_cast<int>(gettid()), ##__VA_ARGS__)


void sighandler1(int signo)
{
	LOG("in sighandler 1, handle %d\n", signo);
	
	if(signo==SIGINT)
		LOG("handle SIGINT\n");
}

void sighandler2(int signo)
{
	LOG("in sighandler 2, handle %d\n", signo);
	
		
}

void Block_USR1()
{
	sigset_t ss, oldss;
	sigemptyset(&ss);
	sigaddset(&ss, SIGUSR1);
	sigprocmask(SIG_BLOCK, &ss, &oldss);
	LOG("BLOCK SIGUSR1\n");
}

// void* ThreadFun1(void* arg)
// {
	// LOG("thread 1 start\n");
	// pthread_detach(pthread_self());
	
	// Block_USR1();
	
	// for(int i=0; i<5; i++)
	// {
		// LOG("wait 50 seconds to handle signal\n");
		// sleep(10);
	// }
	
	// LOG("start handle signal\n");
	
	// sigset_t ss;
	// sigemptyset(&ss);
	// sigaddset(&ss, SIGUSR1);
	// sigaddset(&ss, SIGUSR2);
	// sigaddset(&ss, SIGINT);
	
	// siginfo_t sinfo;
	
	// struct timespec ts;
	// ts.tv_sec=0;
	// ts.tv_nsec=0;
		
	// while(true)
	// {		
		// int signo=sigtimedwait(&ss, &sinfo, &ts);
		// if(signo<0)
		// {
			// LOG("sigtimedeait failed: %d, %s\n", errno, strerror(errno));
			// sleep(10);
		// }
		// else
		// {
			// LOG("obviously handle signo %d\n", signo);
			// sighandler(signo);
		// }
		
	// }
	
	// return 0;
// }

void* ThreadFun1(void* arg)
{
	LOG("thread 1 start\n");
	pthread_detach(pthread_self());
	
	// Block_USR1();
	
	// signal(SIGUSR1, sighandler1);
	// LOG("thread 1 set handler 1  111111111111111111\n");
	
	while(true)
	{
		LOG("thread 1 loop\n");
		
		sleep(5);
	}
	return 0;
	
}

void* ThreadFun2(void* arg)
{
	LOG("thread 2 start\n");
	pthread_detach(pthread_self());
	
	sleep(30);
	signal(SIGUSR1, sighandler2);
	LOG("thread 2 set handler 2  2222222222222222222222222\n");
	
	while(true)
	{
		LOG("thread 2 loop\n");
		
		sleep(5);
	}
	return 0;
	
}

void* ThreadFun3(void* arg)
{
	LOG("thread 2 start\n");
	pthread_detach(pthread_self());
	
	// sleep(100);
	// signal(SIGUSR1, SIG_IGN);
	// LOG("thread 3 set SIG_IGN  IGNIGNIGNIGNIGNIGNIGNIGN \n");
	
	while(true)
	{
		LOG("thread 3 loop\n");
				
		sleep(5);
	}
	return 0;
	
}


int main()
{
	// 主线程阻塞 USR1
	Block_USR1();
	
	pthread_t thread_id;
	int idx1=1;
	pthread_create(&thread_id, NULL, ThreadFun1, &idx1);
	
	int idx2=2;
	pthread_create(&thread_id, NULL, ThreadFun2, &idx2);
	
	int idx3=3;
	pthread_create(&thread_id, NULL, ThreadFun3, &idx2);
	
	

	// signal(SIGINT, sighandler1);
	
	while(true)
	{
		LOG("main loop\n");
		
		sleep(5);
	}
	return 0;
}