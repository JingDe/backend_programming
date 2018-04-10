#include"reactor.muduo/EventLoop.h"

#include"unistd.h"

int cnt = 0;
EventLoop* g_loop;

void print(const char* msg)
{
	printf("msg %lld %s\n", time(NULL), msg);
	if (++cnt == 20)
		g_loop->quit();
}

int main()
{
	EventLoop loop;
	g_loop = &loop;

	loop.runAfter(1, std::bind(print, "once1"));

	loop.loop();
	print("main loop exits");
	sleep(1);

	return 0;
}