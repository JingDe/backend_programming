#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include<sys/types.h>


class EventLoop
{
public:

	static EventLoop* getEventLoopOfCurrentThread();
	EventLoop();
	~EventLoop();

	void loop();
	void quit();

private:
	void AssertInLoopThread();

	bool looping_;
	bool quit_;
	const pid_t pid_;
};

#endif
