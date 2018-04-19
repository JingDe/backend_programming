#include"EPollPoller.h"
#include"logging.muduo/Logging.h"
#include"Channel.h"
#include"testharness.leveldb/testharness.h"

#include<cassert>

#include<unistd.h>
#include<poll.h>
#include<sys/epoll.h>

// ִ�б����ڶ��ԣ�ͬBOOST_STATIC_ASSERT
static_assert((EPOLLIN == POLLIN), "EPOLL != POLLIN");
static_assert((EPOLLPRI == POLLPRI), "EPOLLPRI != POLLPRI");
static_assert((EPOLLOUT == POLLOUT), "EPOLLOUT != POLLOUT");
static_assert((EPOLLRDHUP == POLLRDHUP), "EPOLLRDHUP != POLLRDHUP");
static_assert((EPOLLERR == POLLERR), "EPOLLERR != POLLERR");
static_assert((EPOLLHUP == POLLHUP), "EPOLLHUP != POLLHUP");

const int kNew = -1; // �´�����channel��������channelMap�У�δ��ӵ�epollfd_�й�
const int kAdded = 1; // ����ӵ�epollfd_�У���channelMap��
const int kDeleted = 2; // ��epollfd_��ɾ���ˣ���channelMap��


EPollPoller::EPollPoller(EventLoop* loop) 
	:Poller(loop),
	epollfd_(epoll_create1(EPOLL_CLOEXEC)), // ����һ��epollʵ���������ļ�������
	events_(kInitEventListSize)
{
	/*assert(EPOLLIN == POLLIN);
	assert(EPOLLPRI == POLLPRI);
	ASSERT_TRUE(EPOLLOUT == POLLOUT);
	ASSERT_TRUE(EPOLLRDHUP == POLLRDHUP);
	ASSERT_TRUE(EPOLLERR == POLLERR);
	ASSERT_TRUE(EPOLLHUP == POLLHUP);*/
	if (epollfd_ < 0)
		LOG_FATAL << "EPollPoller::EPollPoller";
}

/*
int epoll_create(int size);
int epoll_create1(int flags);
*/

EPollPoller::~EPollPoller()
{
	close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	LOG_INFO << "fd total count " << channels_.size();
	int numEvents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
	int savedErrno = errno;
	Timestamp now(Timestamp::now());
	if (numEvents > 0)
	{
		LOG_INFO << numEvents << " events happended";
		fillActiveChannels(numEvents, activeChannels);
		if (static_cast<size_t>(numEvents) == events_.size())
			events_.resize(events_.size() * 2); // ��̬��Ӧ�id����Ŀ

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
�ȴ�epfd�ϵ��¼������洢��events�У���෵��maxevents��
maxevents�������0��timeout����-1ʱ��������������0ʱ��������
����ֵ����עIO�¼�׼���õ��ļ��������ĸ�������0������ʱ-1

strutct epoll_event{
	uint32_t events; // ��ע�¼��������¼�
	epoll_data_t data; // �û����ݣ�����ֵ����
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
		channel->set_revents(events_[i].events); // �������ص��¼�
		activeChannels->push_back(channel);
	}
}

void EPollPoller::updateChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	const int index = channel->index(); // Channel��index��ʾ״̬��kNew/kDeleted/kAdded
	LOG_INFO << "fd = " << channel->fd();
	if (index == kNew || index == kDeleted) // kNew��kDeleted��channel������epollfd_�У�ʹ��EPOLL_CTL_ADD���
	{
		int fd = channel->fd();
		if (index == kNew)
		{
			assert(channels_.find(fd) == channels_.end());
			channels_[fd] = channel; // �����channel
		}
		else
		{
			assert(channels_.find(fd) != channels_.end());
			assert(channels_[fd] == channel); // channel map������kDeleted��channel
		}

		channel->set_index(kAdded); // ����channel��״̬
		update(EPOLL_CTL_ADD, channel); // epoll����״̬�ģ�ά���ں����fd״̬��Ӧ�ó����״̬��ͬ
	}
	else // kAdded��channel����epollfd_�У�ʹ��EPOLL_CTL_MOD��EPOLL_CTL_DEL�޸Ļ�ɾ��
	{
		int fd = channel->fd();
		assert(channels_.find(fd) != channels_.end());
		assert(channels_[fd] = channel);
		assert(index == kAdded);
		if (channel->isNoneEvent())
		{
			update(EPOLL_CTL_DEL, channel);
			channel->set_index(kDeleted);
		}
		else
			update(EPOLL_CTL_MOD, channel);
	}
}

// epoll����״̬�ģ�ά���ں����fd״̬��Ӧ�ó����״̬��ͬ
void EPollPoller::update(int operation, Channel* channel)
{
	struct epoll_event event;
	bzero(&event, sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;
	int fd = channel->fd();
	LOG_INFO << "epoll_ctl op = " << operationToString(operation)
		<< " fd = " << fd << " event = { " << channel->eventsToString() << " }";
	if (epoll_ctl(epollfd_, operation, fd, &event) < 0)
	{
		if (operation == EPOLL_CTL_DEL)
		{
			LOG_ERROR << "epoll_ctl op=" << operationToString(operation);
		}
		else
		{
			LOG_FATAL << "epoll_ctl op=" << operationToString(operation);
		}
	}
}

/*
int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
*/


// ����һ��ֵͬ��ʼ��һ���������βεķ�ʽһ�������ص�ֵ���ڳ�ʼ�����õ��һ����ʱ��
// ���ַ��������������ַ��������ظ��ַ����ĵ�ַ���������õ��øõ�ַ
const char* EPollPoller::operationToString(int op)
{
	switch (op)
	{
	case EPOLL_CTL_ADD:
		return "ADD";
	case EPOLL_CTL_DEL:
		return "DEL";
	case EPOLL_CTL_MOD:
		return "MODE";
	default:
		//assert(false && "ERROR op");
		return "Unknown Operation";
	}
}

void EPollPoller::removeChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	int fd = channel->fd();
	LOG_INFO << "fd = " << fd;
	assert(channels_.find(fd) != channels_.end());
	assert(channels_[fd] == channel);
	assert(channel->isNoneEvent());
	int index = channel->index();
	assert(index == kAdded || index == kDeleted);
	size_t n = channels_.erase(fd);
	assert(n == 1);

	if(index==kAdded)
		update(EPOLL_CTL_DEL, channel);
	channel->set_index(kNew);
}