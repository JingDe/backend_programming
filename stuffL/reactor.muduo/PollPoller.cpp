#include"PollPoller.h"
#include"logging.muduo/Logging.h"
#include"Channel.h"

#include<algorithm>
#include<cassert>

#include<poll.h>

PollPoller::PollPoller(EventLoop* loop)
	:Poller(loop) // 调用基类的构造函数
{

}

PollPoller::~PollPoller()
{}

time_t PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	int numEvents=::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
	int savedErrno = errno;
	time_t now = time(NULL);
	if (numEvents > 0)
	{
		LOG_INFO << numEvents << " events happened";
		fillActiveChannels(numEvents, activeChannels);
	}
	else if (numEvents == 0)
		LOG_INFO << " ";
	else
	{
		if (savedErrno != EINTR)
		{
			errno = savedErrno;
			LOG_ERROR << "PollPoller::poll()";
		}
	}
	return now;
}

// poll调用之后，revents被设置
// std::vector<struct pollfd> --->> std::vector<Channel*>
void PollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels)
{
	for (PollFdList::const_iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd)
	{
		if (pfd->revents > 0)
		{
			--numEvents;
			//ChannelMap::iterator it = find(channels_.begin(), channels_.end(), pfd->fd);
			ChannelMap::const_iterator it = channels_.find(pfd->fd);
			assert(it != channels_.end());
			Channel* channel = it->second;
			channel->set_revents(pfd->revents);
			activeChannels->push_back(channel);
		}
	}
}

void PollPoller::updateChannel(Channel* c)
{}

void PollPoller::removeChannel(Channel* c)
{

}