#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include<sys/types.h>
#include<memory>
#include<vector>

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

	void assertInLoopThread();

	void updateChannel(Channel* c);

private:	
	void printActiveChannels() const;
	void doPendingFunctors();

	bool looping_;
	bool quit_;
	const pid_t pid_;

	std::auto_ptr<Poller> poller_;

	typedef std::vector<Channel*> ChannelList;
	ChannelList activeChannels_;

	Channel* currentActiveChannel_;

	time_t pollReturnTime_;
};

#endif
