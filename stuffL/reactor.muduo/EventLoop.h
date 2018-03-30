#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

class EventLoop
{
public:
	EventLoop();


private:
	bool looping_;
	const pid_t pid_;
};

#endif
