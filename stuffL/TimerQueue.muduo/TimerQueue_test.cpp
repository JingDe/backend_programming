#include"reactor.muduo/EventLoop.h"
#include"TimerId.h"

#include<unistd.h>

#include"logging.muduo/Logging.h"

int cnt = 0;
EventLoop* g_loop;

void print(const char* msg)
{
	printf("msg %lld %s\n", time(NULL), msg);

	if (++cnt == 20)
		g_loop->quit();
}


void test()
{
	Logger::setLogLevel(Logger::DEBUG);

	EventLoop loop;
	g_loop = &loop;


	loop.runEvery(5, std::bind(print, "every5"));

	loop.runAfter(10, std::bind(print, "once10"));


	loop.loop();
	print("main loop exits");
	sleep(1);

}

int main()
{
	test();

	return 0;
}