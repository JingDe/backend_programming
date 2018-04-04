#include"Channel.h"
#include"logging.muduo/Logging.h"
#include"EventLoop.h"

#include<sstream>

#include"poll.h"


//class EventLoop; // �õ�EventLoop*������ҪEventLoop�����������

const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;
const int Channel::kNoneEvent = 0;

Channel::Channel(EventLoop* loop, int fd) 
	:fd_(fd), index_(-1),loop_(loop), tied_(false), eventHandling_(false),events_(0),revents_(0)
{}

Channel::~Channel()
{

}

// handleEvent()�ڻص��û�onClose����ʱ���п�������Channel��������
// tie�����ӳ�Channel�������owner����������ڣ�ʹ�䳤��handleEvent����
void Channel::tie(const std::shared_ptr<void>& obj) 
{
	tie_ = obj; // weak_ptr& operator=( const shared_ptr<Y>& r ) noexcept;
	tied_ = true;
}

void Channel::handleEvent(time_t receiveTime)
{
	std::shared_ptr<void> guard;
	if (tied_)
	{
		guard = tie_.lock(); // guard�����ӳ�Channel�������owner����������ڣ�ʹ�䳤��handleEvent����
		if (guard)
			handleEventWithGuard(receiveTime);
	}
	else
		handleEventWithGuard(receiveTime);
}

void Channel::handleEventWithGuard(time_t receiveTime)
{
	eventHandling_ = true;
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) // socket�����رչرշ���POLLHUP�������쳣�Ͽ�����POLLIN��read����0??
	{
		LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
		if (closeCallback_)
			closeCallback_();
	}

	if (revents_  &  POLLNVAL)
		LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";

	if (revents_ & (POLLERR | POLLNVAL))
		if (errorCallback_)
			errorCallback_();

	if (revents_  &  (POLLIN | POLLPRI | POLLRDHUP)) // �����ݶ����������ݶ���socket�Զ˹ر����ӻ�رն���
		if (readCallback_)
			readCallback_(receiveTime);

	if (revents_  &  POLLOUT)
		if (writeCallback_)
			writeCallback_();
	eventHandling_ = false;
}

void Channel::update()
{
	loop_->updateChannel(this);
}

std::string Channel::eventsToString() const
{
	return eventsToString(fd_, events_);
}

std::string Channel::reventsToString() const
{
	return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString(int fd, int ev) const
{
	std::ostringstream oss;
	oss << fd << ": ";
	if (ev  &  POLLIN)
		oss << "IN ";
	if (ev  &  POLLPRI)
		oss << "PRI ";
	if (ev  &  POLLOUT)
		oss << "OUT ";
	if (ev  &  POLLHUP)
		oss << "HUP ";
	if (ev  &  POLLRDHUP)
		oss << "RDHUP";
	if (ev  &  POLLERR)
		oss << "ERR ";
	if (ev  &  POLLNVAL)
		oss << "NVAL ";
	return oss.str().c_str();
}