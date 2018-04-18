#ifndef POLLPOLLER_H_
#define POLLPOLLER_H_

#include"Poller.h"

class PollPoller : public Poller {
public:
	PollPoller(EventLoop* loop);
	virtual ~PollPoller();

	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);

	virtual void updateChannel(Channel* c);
	virtual void removeChannel(Channel* c);

private:
	void fillActiveChannels(int numEvents, ChannelList* activeChannels);

	typedef std::vector<struct pollfd> PollFdList;
	PollFdList pollfds_;
};

/*
int poll(struct pollfd* fds, nfds_t nfds, int timeout);
返回值：有非0的revents的结构数；0表示超时；-1出错

struct pollfd {
	int fd;
	short events;
	short revents; // 由内核设置的事件，可以包含events或者POLLERR, POLLHUP, POLLNVAL
};
*/

#endif