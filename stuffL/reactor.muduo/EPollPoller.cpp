#include"EPollPoller.h"
#include"logging.muduo/Logging.h"
#include"Channel.h"
#include"test.leveldb/testharness.h"

#include<cassert>

#include<unistd.h>
#include<poll.h>
#include<sys/epoll.h>

// 执行编译期断言，同BOOST_STATIC_ASSERT
static_assert((EPOLLIN == POLLIN), "EPOLL != POLLIN");
static_assert((EPOLLPRI == POLLPRI), "EPOLLPRI != POLLPRI");
static_assert((EPOLLOUT == POLLOUT), "EPOLLOUT != POLLOUT");
static_assert((EPOLLRDHUP == POLLRDHUP), "EPOLLRDHUP != POLLRDHUP");
static_assert((EPOLLERR == POLLERR), "EPOLLERR != POLLERR");
static_assert((EPOLLHUP == POLLHUP), "EPOLLHUP != POLLHUP");

const int kNew = -1; // 新创建的channel，不存在channelMap中，未添加到epollfd_中过
const int kAdded = 1; // 已添加到epollfd_中，在channelMap中
const int kDeleted = 2; // 从epollfd_中删除了，在channelMap中


EPollPoller::EPollPoller(EventLoop* loop) 
	:Poller(loop),
	epollfd_(epoll_create1(EPOLL_CLOEXEC)), // 创建一个epoll实例，返回文件描述符
	events_(kInitEventListSize)
{
	/*assert(EPOLLIN == POLLIN);
	assert(EPOLLPRI == POLLPRI);
	ASSERT_TRUE(EPOLLOUT == POLLOUT);
	ASSERT_TRUE(EPOLLRDHUP == POLLRDHUP);
	ASSERT_TRUE(EPOLLERR == POLLERR);
	ASSERT_TRUE(EPOLLHUP == POLLHUP);*/
}

/*
int epoll_create(int size);
int epoll_create1(int flags);
*/

EPollPoller::~EPollPoller()
{
	close(epollfd_);
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
等待epfd上的事件，maxevents必须大于0，timeout等于-1时无限阻塞，等于0时立即返回
返回值：关注IO事件准备好的文件描述符的个数，或0，出错时-1

strutct epoll_event{
	uint32_t events; // 关注事件，返回事件
	epoll_data_t data; // 用户数据，返回值不变
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
		channel->set_revents(events_[i].events); // 包含返回的事件
		activeChannels->push_back(channel);
	}
}

void EPollPoller::updateChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	const int index = channel->index(); // Channel的index表示状态：kNew/kDeleted/kAdded
	LOG_INFO << "fd = " << channel->fd();
	if (index == kNew || index == kDeleted) // kNew或kDeleted的channel不存在epollfd_中，使用EPOLL_CTL_ADD添加
	{
		int fd = channel->fd();
		if (index == kNew)
		{
			assert(channels_.find(fd) == channels_.end());
			channels_[fd] = channel; // 添加新channel
		}
		else
		{
			assert(channels_.find(fd) != channels_.end());
			assert(channels_[fd] == channel); // channel map里已有kDeleted的channel
		}

		channel->set_index(kAdded); // 更新channel的状态
		update(EPOLL_CTL_ADD, channel);
	}
	else // kAdded的channel存在epollfd_中，使用EPOLL_CTL_MOD或EPOLL_CTL_DEL修改或删除
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


// 返回一个值同初始化一个变量或形参的方式一样：返回的值用于初始化调用点的一个临时量
// 在字符串常量区创建字符串，返回该字符串的地址，函数调用点获得该地址
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