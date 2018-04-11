#include"Poller.h"
#include"PollPoller.h"
#include"EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
	return new PollPoller(loop);
	//return new EPollPoller(loop);
}