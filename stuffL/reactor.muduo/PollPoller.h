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
����ֵ���з�0��revents�Ľṹ����0��ʾ��ʱ��-1����

struct pollfd {
	int fd;
	short events;
	short revents; // ���ں����õ��¼������԰���events����POLLERR, POLLHUP, POLLNVAL
};
*/

#endif