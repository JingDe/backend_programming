#include"issues/test_timestamp/EventLoop_TS.h"
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

void cancel(TimerId timer)
{
	g_loop->cancel(timer);
	time_t now = time(NULL);
	printf("cancelled at %lld\n", now);
}

void test1()
{
	Logger::setLogLevel(Logger::DEBUG);

	EventLoop loop;
	g_loop = &loop;


	loop.runAfter(1, std::bind(print, "once1"));
	loop.runAfter(1.5, std::bind(print, "once1.5"));
	loop.runAfter(2.5, std::bind(print, "once2.5"));
	loop.runAfter(3.5, std::bind(print, "once3.5"));
	TimerId t45 = loop.runAfter(4.5, std::bind(print, "once4.5"));
	loop.runAfter(4.2, std::bind(cancel, t45));
	loop.runAfter(4.8, std::bind(cancel, t45));
	loop.runEvery(2, std::bind(print, "every2"));
	TimerId t3 = loop.runEvery(3, std::bind(print, "every3"));
	loop.runAfter(9.001, std::bind(cancel, t3));

	loop.loop();
	print("main loop exits");
	sleep(1);

}

void test2()
{

}

int main()
{
	sleep(1);
	test1();

	return 0;
}