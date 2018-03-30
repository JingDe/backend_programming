#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include<sys/types.h>


class EventLoop
{
public:
	EventLoop();
	~EventLoop();

private:
	bool looping_;
	const pid_t pid_;
};

#endif
