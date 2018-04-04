#include"Channel.h"
#include"logging.muduo/Logging.h"
#include"EventLoop.h"

#include<sstream>

#include"poll.h"


//class EventLoop; // 用到EventLoop*，不需要EventLoop类的完整定义

const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;
const int Channel::kNoneEvent = 0;

Channel::Channel(EventLoop* loop, int fd) 
	:fd_(fd), index_(-1),loop_(loop), tied_(false), eventHandling_(false),events_(0),revents_(0)
{}

Channel::~Channel()
{

}

// handleEvent()在回调用户onClose函数时，有可能析构Channel对象自身
// tie函数延长Channel对象或其owner对象的生命期，使其长过handleEvent函数
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
		guard = tie_.lock(); // guard对象延长Channel对象或其owner对象的生命期，使其长过handleEvent函数
		if (guard)
			handleEventWithGuard(receiveTime);
	}
	else
		handleEventWithGuard(receiveTime);
}

void Channel::handleEventWithGuard(time_t receiveTime)
{
	eventHandling_ = true;
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) // socket正常关闭关闭返回POLLHUP，连接异常断开返回POLLIN，read返回0??
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

	if (revents_  &  (POLLIN | POLLPRI | POLLRDHUP)) // 有数据读、紧急数据读、socket对端关闭连接或关闭读端
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