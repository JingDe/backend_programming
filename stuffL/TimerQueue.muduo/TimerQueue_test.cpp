#include"reactor.muduo/EventLoop.h"

EventLoop* g_loop;

int main()
{
	EventLoop loop;
	g_loop = &loop;

	loop.runAfter();
}