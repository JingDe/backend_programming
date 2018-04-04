#ifndef EPOLLPOLLER_H_
#define EPOLLPOLLER_H_

#include"Poller.h"

#include<sys/epoll.h>

class EPollPoller : public Poller {
public:
	EPollPoller(EventLoop* loop);
	virtual ~EPollPoller();

	virtual time_t poll(int timeoutMs, ChannelList* activeChannels);


private:
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
	
	static const int kInitEventListSize = 16;

	int epollfd_;

	typedef std::vector<struct epoll_event> EventList;
	EventList events_;
};

#endif 
