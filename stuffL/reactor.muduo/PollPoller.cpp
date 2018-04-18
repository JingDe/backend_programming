#include"PollPoller.h"
#include"logging.muduo/Logging.h"
#include"Channel.h"
#include"TimerQueue.muduo/Timestamp.h"

#include<algorithm>
#include<cassert>
#include<functional>

#include<poll.h>

PollPoller::PollPoller(EventLoop* loop)
	:Poller(loop) // ���û���Ĺ��캯��
{

}

PollPoller::~PollPoller()
{}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	int numEvents=::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
	int savedErrno = errno;
	Timestamp now(Timestamp::now());
	if (numEvents > 0)
	{
		LOG_INFO << numEvents << " events happened";
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
			LOG_ERROR << "PollPoller::poll()";
		}
	}
	return now;
}

// poll����֮��revents������
// std::vector<struct pollfd> --->> std::vector<Channel*>
void PollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels)
{
	for (PollFdList::iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd)
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

			pfd->revents = 0; // ??
		}
	}
}

// ͬʱ����channels_��pollfds_
void PollPoller::updateChannel(Channel* c)
{
	Poller::assertInLoopThread();
	int fd = c->fd();
	LOG_INFO << "fd = " << fd << " events = " << c->events();
	//ChannelMap::const_iterator it = channels_.find(fd); // ʹ��index��channel��PollFdList pollfds_�е��±꣩������ҿ���
	if(c->index()<0)	//if (it == channels_.end())
	{
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = c->events();
		pfd.revents = c->revents();
		pollfds_.push_back(std::move(pfd));

		c->set_index(pollfds_.size()-1);
		//channels_.insert(std::make_pair(fd, c));
		channels_[fd] = c; // �޸Ļ����
	}
	else
	{
		//channels_[fd] = c;

		/*std::vector<struct pollfd>::iterator it = find(pollfds_.begin(), pollfds_.end(), 
			[fd](const struct pollfd pfd) -> bool {
			if (fd == pfd.fd)
				return true;
			else 
				return false;
			});
		assert(it != pollfds_.end());
		it->events = c->events();
		it->revents = c->revents();*/
		int idx = c->index();
		assert(idx >= 0 && idx < static_cast<int>(pollfds_.size()));
		struct pollfd& pfd = pollfds_[idx];
		assert(pfd.fd == fd  ||  pfd.fd==-c->fd()-1);
		pfd.events = static_cast<short>(c->events());
		pfd.revents = c->revents();

		if (c->isNoneEvent())
			pollfds_[idx].fd = -c->fd() - 1;
	}
}

void PollPoller::removeChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	LOG_INFO << "fd = " << channel->fd();

	int idx = channel->index();
	assert(idx >= 0 && idx < pollfds_.size());
	const struct pollfd& pfd = pollfds_[idx];
	assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events()); // 

	size_t n=channels_.erase(channel->fd());
	assert(n == 1);


	// pollfds_.erase(pollfds_.begin()+channel->index()); // vector::erase���Ӷ�O(n)
	if (idx == pollfds_.size() - 1)
		pollfds_.pop_back();
	else
	{
		int channelAtEnd = pollfds_.back().fd;
		// ɾ��idxλ��Ԫ�ظ��Ӷ�O(1):��������������ָ���ֵ����ɾ�����һ��Ԫ��
		iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1); 
		if (channelAtEnd < 0)
			channelAtEnd = -channelAtEnd - 1;
		channels_[channelAtEnd]->set_index(idx); // ???
		pollfds_.pop_back();
	}
}