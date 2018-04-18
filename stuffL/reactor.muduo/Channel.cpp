#include"Channel.h"
#include"logging.muduo/Logging.h"
#include"EventLoop.h"
#include"TimerQueue.muduo/Timestamp.h"

#include<sstream>
#include<cassert>

#include"poll.h"


//class EventLoop; // 用到EventLoop*，不需要EventLoop类的完整定义

const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;
const int Channel::kNoneEvent = 0;

Channel::Channel(EventLoop* loop, int fd) 
	:loop_(loop), 
	fd_(fd), 
	events_(0), 
	revents_(0), 
	index_(-1),
	tied_(false), 
	eventHandling_(false),
	addedToLoop_(false)
{}

Channel::~Channel()
{
	assert(!addedToLoop_); // 保证在析构之前已从loop移除
	assert(!eventHandling_); // 保证在事件处理handleEvent期间不会析构
}

// handleEvent()在回调用户onClose函数时，有可能析构Channel对象自身
// tie函数延长Channel对象或其owner对象的生命期，使其长过handleEvent函数
void Channel::tie(const std::shared_ptr<void>& obj) 
{
	tie_ = obj; // weak_ptr& operator=( const shared_ptr<Y>& r ) noexcept;
	tied_ = true;
}

void Channel::handleEvent(Timestamp receiveTime)
{
	std::shared_ptr<void> guard;
	if (tied_)
	{
		guard = tie_.lock(); // guard对象延长Channel对象或其owner对象的生命期，使其长过handleEventWithGuard函数
		if (guard)
			handleEventWithGuard(receiveTime);
	}
	else
		handleEventWithGuard(receiveTime);
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	eventHandling_ = true;
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) // socket正常关闭返回POLLHUP，连接异常断开返回POLLIN并且read返回0??
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
	addedToLoop_ = true;
	loop_->updateChannel(this);
}

void Channel::remove()
{
	assert(isNoneEvent());
	addedToLoop_ = false;
	loop_->removeChannel(this);
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

