#include"EPollPoller.h"
#include"logging.muduo/Logging.h"
#include"Channel.h"

#include<cassert>



EPollPoller::EPollPoller(EventLoop* loop) 
	:Poller(loop)
{

}

EPollPoller::~EPollPoller()
{

}

time_t EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	LOG_INFO << "fd total count " << channels_.size();
	int numEvents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
	int savedErrno = errno;
	time_t now = time(NULL);
	if (numEvents > 0)
	{
		LOG_INFO << numEvents << " events happended";
		fillActiveChannels(numEvents, activeChannels);
	}
	else if (numEvents == 0)
	{
		LOG_INFO << "nothing happened";
	}
	else
	{
		if (savedErrno != EINTR)
		{
			errno = savedErrno;
			LOG_ERROR << "EPollPoller::poll()";
		}
	}
	return now;
}

/*
int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);

strutct epoll_event{
	uint32_t events; // 关注事件
	epoll_data_t data; // 用户数据
};

typedef union epoll_data{
	void* ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;
}epoll_data_t;

*/

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
	assert(static_cast<size_t>(numEvents) <= events_.size());
	for (int i = 0; i < numEvents; i++)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
		int fd = channel->fd();
		ChannelMap::const_iterator it = channels_.find(fd);
		assert(it != channels_.end());
		assert(it->second == channel);
#endif
		channel->set_revents(events_[i].events);
		activeChannels->push_back(channel);
	}
}