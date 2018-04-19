#ifndef EPOLLPOLLER_H_
#define EPOLLPOLLER_H_

#include"Poller.h"

#include<sys/epoll.h>

class EPollPoller : public Poller {
public:
	EPollPoller(EventLoop* loop);
	virtual ~EPollPoller();

	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
	virtual void updateChannel(Channel* channel);
	virtual void removeChannel(Channel* channel);

private:
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
	void update(int operation, Channel* channel);
	const char* operationToString(int operation);
	
	static const int kInitEventListSize = 16;

	// ChannelMap channels_; 在EPollPoller中，channel.index被用为标记channel是否在epoll的关注列表中
	int epollfd_;
	typedef std::vector<struct epoll_event> EventList;
	EventList events_;
};

#endif 
