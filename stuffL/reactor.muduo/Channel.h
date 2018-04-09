#ifndef CHANNEL_H_
#define CHANNEL_H_

#include<ctime>
#include<memory>
#include<string>

class EventLoop;


class Channel {
public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(time_t)> ReadEventCallback;

	Channel(EventLoop* loop, int fd);
	~Channel();

	void tie(const std::shared_ptr<void>& obj);
	void set_revents(int revt) { revents_ = revt; }
	void set_index(int index) { index_ = index;  }

	void handleEvent(time_t receiveTime);

	void update();
	void enableReading() { events_ |= kReadEvent; update(); }
	void enableWriting() { events_ |= kWriteEvent; update(); }
	void disableAll() { events_ = kNoneEvent; update(); }

	bool isNoneEvent() const { return events_ == kNoneEvent;  }

	int fd() const { return fd_;  }
	int index() const { return index_;  }
	int events() const { return events_;  }
	int revents() const { return revents_;  }
	EventLoop* ownerLoop() { return loop_;  }

	std::string eventsToString() const;
	std::string reventsToString() const;

	void setReadCallback(ReadEventCallback cb) { readCallback_ = cb;  }

	void remove();

private:
	void handleEventWithGuard(time_t receiveTime);
	std::string eventsToString(int fd, int ev) const;

	const int fd_; // Channel������ļ�������
	int events_; // ���ĵ��¼�
	int revents_; // epoll��poll���ܵ��¼�

	int index_; // ��PollPoller���б�ʾ��Channel��PollPoller��pollfd_�е��±�
				// ��EPollPoller���б�ʾkNew kAdded kDeleted

	EventLoop* loop_;
	bool tied_;
	std::weak_ptr<void> tie_;
	bool eventHandling_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;

	bool addedToLoop_;

	static const int kReadEvent; // �����¼� POLLIN |  POLLPRI
	static const int kWriteEvent; // ����¼� POLLOUT
	static const int kNoneEvent; // 0
};

/*
int poll(struct pollfd* fds, nfds_t nfds, int timeout);
struct pollfd{
	int fd;
	short events;
	short revents; // ���ں����õ��¼������԰���events����POLLERR, POLLHUP, POLLNVAL
};
POLLERR:��������
POLLHUP:hang up
POLLNVAL:fd���Ǵ򿪵�������

*/

#endif
