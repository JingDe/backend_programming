#include"EventLoop.h"
#include"EventLoopThread.h"

#include<unistd.h>

#include"thread.muduo/thread_util.h"
#include"TimerQueue.muduo/TimerId.h"

void runInThread()
{
	printf("runInThread(): pid=%d, tid=%d\n", getpid(), gettid());
}

int main()
{
	EventLoopThread loopThread;
	EventLoop* loop = loopThread.startLoop();
	loop->runInLoop(runInThread);
	sleep(1);
	loop->runAfter(2, runInThread);
	sleep(3);
	loop->quit();

	printf("exit main()\n");

	return 0;
}