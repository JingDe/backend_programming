#ifndef CHANNEL_H_
#define CHANNEL_H_

#include<ctime>
#include<memory>

class EventLoop;


class Channel {
public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(time_t)> ReadCallback;

	Channel(int fd, EventLoop* loop) :fd_(fd),loop_(loop),tied_(false), eventHandling_(false){}
	~Channel();

	void tie(const std::shared_ptr<void>& obj);
	void set_revents(int revt) { revents_ = revt; }
	void handleEvent(time_t receiveTime);

	void update();
	void enableReading() { events_ |= kReadEvent; update(); }
	void enableWriting() { events_ |= kWriteEvent; update(); }

	EventLoop* ownerLoop() { return loop_;  }

private:
	void handleEventWithGuard(time_t receiveTime);

	const int fd_; // Channel管理的文件描述符
	int events_; // 关心的事件
	int revents_; // epoll或poll接受的事件

	EventLoop* loop_;
	bool tied_;
	std::weak_ptr<void> tie_;
	bool eventHandling_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
	ReadCallback readCallback_;
	EventCallback writeCallback_;

	static const int kReadEvent;
	static const int kWriteEvent;
	static const int kNoneEvent;
};

/*
int poll(struct pollfd* fds, nfds_t nfds, int timeout);
struct pollfd{
	int fd;
	short events;
	short revents; // 由内核设置的事件，可以包含events或者POLLERR, POLLHUP, POLLNVAL
};
POLLERR:发生错误
POLLHUP:hang up
POLLNVAL:fd不是打开的描述符

*/

#endif
