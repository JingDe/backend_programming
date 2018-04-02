#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include<sys/types.h>

class Channel;
class Poller;

class EventLoop
{
public:

	static EventLoop* getEventLoopOfCurrentThread();
	EventLoop();
	~EventLoop();

	void loop();
	void quit();

	void updateChannel(Channel* c);

private:
	void AssertInLoopThread();

	bool looping_;
	bool quit_;
	const pid_t pid_;

	std::auto_ptr<Poller> poller_;
};

#endif
