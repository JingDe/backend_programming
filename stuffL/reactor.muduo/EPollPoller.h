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

	// ChannelMap channels_; ��EPollPoller�У�channel.index����Ϊ���channel�Ƿ���epoll�Ĺ�ע�б���
	int epollfd_;
	typedef std::vector<struct epoll_event> EventList;
	EventList events_;
};

#endif 
