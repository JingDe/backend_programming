#include"EventLoop.h"
#include"EventLoopThread.h"

#include<unistd.h>

#include"thread.muduo/thread_util.h"
#include"TimerQueue.muduo/TimerId.h"

void runInThread()
{
	printf("runInThread(): pid=%d, tid=%d\n", getpid(), gettid());
}

void print(EventLoop* p = NULL)
{
	printf("print: pid = %d, tid = %d, loop = %p\n",
		getpid(), gettid(), p);
}

void quit(EventLoop* loop)
{
	print(loop);
	loop->quit();
}

int main()
{
	{
		EventLoopThread loopThread;
		EventLoop* loop = loopThread.startLoop();
		loop->runInLoop(runInThread);
		sleep(1);
		loop->runAfter(2, runInThread);
		sleep(3);
		loop->quit(); // quitÖ®ºóloop==NULL
	}
	
	{
		EventLoopThread thr2;
		EventLoop* loop = thr2.startLoop();
		loop->runInLoop(std::bind(print, loop));
		sleep(1);
	}

	{
		EventLoopThread thr3;
		EventLoop* loop = thr3.startLoop();
		loop->runInLoop(std::bind(quit, loop));
		sleep(1);
	}
	printf("exit main()\n");

	return 0;
}