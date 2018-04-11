#include"reactor.muduo/EventLoop.h"
#include"TimerId.h"

#include<unistd.h>

#include"logging.muduo/Logging.h"

int cnt = 0;
EventLoop* g_loop;

void print(const char* msg)
{
	printf("msg %lld %s\n", time(NULL), msg);
	LOG_DEBUG << "msg " << time(NULL) << " " << msg;
	if (++cnt == 20)
		g_loop->quit();
}

int main()
{
	Logger::setLogLevel(Logger::DEBUG);

	EventLoop loop;
	g_loop = &loop;

	loop.runAfter(1, std::bind(print, "once1"));
	loop.runAfter(1.5, std::bind(print, "once1.5"));
	loop.runAfter(2.5, std::bind(print, "once2.5"));
	loop.runAfter(3.5, std::bind(print, "once3.5"));
	loop.runEvery(2, std::bind(print, "once2"));
	loop.runEvery(3, std::bind(print, "once3"));

	loop.runAfter(5, std::bind(print, "once5"));
	loop.runAfter(10, std::bind(print, "once10"));
	loop.runEvery(6, std::bind(print, "once6"));
	loop.runEvery(8, std::bind(print, "once8"));

	loop.loop();
	print("main loop exits");
	sleep(1);

	return 0;
}