#include"EventLoopThreadPool.h"
#include"EventLoop.h"

#include<cassert>

#include<unistd.h>

#include"thread.muduo/thread_util.h"
#include"TimerQueue.muduo/TimerId.h"
#include"logging.muduo/Logging.h"

void print(EventLoop* p = NULL)
{
	printf("main(): pid = %d, tid = %d, loop = %p\n",
		getpid(), gettid(), p);
}

void init(EventLoop* p)
{
	printf("init(): pid = %d, tid = %d, loop = %p\n",
		getpid(), gettid(), p);
}

int main()
{
	Logger::setLogLevel(Logger::DEBUG);

	print();

	EventLoop loop;
	loop.runAfter(11, std::bind(&EventLoop::quit, &loop));

	{
		printf("Single thread %p:\n", &loop);
		EventLoopThreadPool model(&loop);
		model.setThreadNum(0);
		model.start(init);
		assert(model.getNextLoop() == &loop);
		assert(model.getNextLoop() == &loop);
	}

	{
		printf("Another thread:\n");
		EventLoopThreadPool model(&loop);
		model.setThreadNum(1);
		model.start(init);
		EventLoop* nextLoop = model.getNextLoop();
		nextLoop->runAfter(2, std::bind(print, nextLoop));
		assert(nextLoop != &loop);
		assert(nextLoop == model.getNextLoop());
		assert(nextLoop == model.getNextLoop());
		sleep(3);
	}
	printf("----------------------\n");

	{
		printf("Three threads:\n");
		EventLoopThreadPool model(&loop);
		model.setThreadNum(3);
		model.start(init);
		EventLoop* nextLoop = model.getNextLoop();
		nextLoop->runInLoop(std::bind(print, nextLoop));
		assert(nextLoop != &loop);
		assert(nextLoop != model.getNextLoop());
		assert(nextLoop != model.getNextLoop());
		assert(nextLoop == model.getNextLoop());
	}
	loop.loop();

	return 0;
}