#include"Connector.h"
#include"reactor.muduo/EventLoop.h"
#include"TimerQueue.muduo/TimerId.h"

#include<cassert>

#include"logging.muduo/Logging.h"

const int Connector::kMaxRetryDelayMs;
const int Connector::kInitRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
	:loop_(loop),
	serverAddr_(serverAddr),
	connect_(false),
	state_(kDisconnected),
	retryDelayMs_(kInitRetryDelayMs)
{
	LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
	LOG_DEBUG << "dtor[" << this << "]";
	assert(!channel_);
}

void Connector::start()
{
	connect_ = true;
	loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
	loop_->assertInLoopThread();
	assert(state_ == kDisconnected);
	if (connect_)
	{
		connect();
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

void Connector::stop()
{
	connect_ = false;
	loop_->queueInLoop(std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
																   // FIXME: cancel timer
}

void Connector::stopInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kConnecting)
	{
		setState(kDisconnected);
		removeAndResetChannel();
		retry(sockfd_);
	}
}

void Connector::restart()
{
	loop_->assertInLoopThread();
	setState(kDisconnected);
	retryDelayMs_ = kInitRetryDelayMs;
	connect_ = true;
	startInLoop();
}

void Connector::connect()
{
	sockfd_ = sockets::createNonblockingOrDie(serverAddr_.family());
	int ret = sockets::connect(sockfd_, serverAddr_.getSockAddr());
	int savedErrno = (ret == 0) ? 0 : errno;
	switch (savedErrno)
	{
	case 0:
	case EINPROGRESS: // 正在连接
	case EINTR:
	case EISCONN:
		connecting(sockfd_);
		break;

	case EAGAIN: // 端口用完，关闭socket重试
	case EADDRINUSE:
	case EADDRNOTAVAIL:
	case ECONNREFUSED:
	case ENETUNREACH:
		retry(sockfd_);
		break;

	case EACCES:
	case EPERM:
	case EAFNOSUPPORT:
	case EALREADY:
	case EBADF:
	case EFAULT:
	case ENOTSOCK:
		LOG_ERROR << "connect error in Connector::startInLoop " << savedErrno;
		sockets::close(sockfd_);
		break;

	default:
		LOG_ERROR << "Unexpected error in Connector::startInLoop " << savedErrno;
		sockets::close(sockfd_);
		// connectErrorCallback_();
		break;
	}
}

void Connector::connecting(int sockfd)
{
	setState(kConnecting);
	assert(!channel_);
	channel_.reset(new Channel(loop_, sockfd));
	channel_->setWriteCallback(std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
	channel_->setErrorCallback(std::bind(&Connector::handleError, this)); // FIXME: unsafe

	// channel_->tie(shared_from_this()); is not working, as channel_ is not managed by shared_ptr
	channel_->enableWriting();
}

// 取消关注channel，删除channel
void Connector::removeAndResetChannel()
{
	channel_->disableAll();
	channel_->remove();
	// Can't reset channel_ here, because we are inside Channel::handleEvent
	loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
}

void Connector::resetChannel()
{
	channel_.reset();
}


void Connector::handleWrite()
{
	LOG_TRACE << "Connector::handleWrite " << state_;

	if (state_ == kConnecting)
	{
		removeAndResetChannel();
		int err = sockets::getSocketError(sockfd_); // socket可写，不意味着连接已成功建立
		if (err)
		{
			LOG_WARN << "Connector::handleWrite - SO_ERROR = "
				<< err << " " << strerror(err);
			retry(sockfd_);
		}
		else if (sockets::isSelfConnect(sockfd_)) // 自连接
		{
			LOG_WARN << "Connector::handleWrite - Self connect";
			retry(sockfd_);
		}
		else
		{
			setState(kConnected);
			if (connect_)
			{
				newConnectionCallback_(sockfd_);
			}
			else
			{
				sockets::close(sockfd_);
			}
		}
	}
	else
	{
		// what happened?
		assert(state_ == kDisconnected);
	}
}

void Connector::handleError()
{
	LOG_ERROR << "Connector::handleError state=" << state_;
	if (state_ == kConnecting)
	{
		removeAndResetChannel();
		int err = sockets::getSocketError(sockfd_);
		LOG_TRACE << "SO_ERROR = " << err << " " << strerror(err);
		retry(sockfd_);
	}
}

void Connector::retry(int sockfd)
{
	sockets::close(sockfd);
	setState(kDisconnected);
	if (connect_)
	{
		LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
			<< " in " << retryDelayMs_ << " milliseconds. ";
		loop_->runAfter(retryDelayMs_ / 1000.0,
			std::bind(&Connector::startInLoop, shared_from_this()));
		// shared_from_this延长本Connector对象生命期，在startInLoop期间有效
		// 方式2：Connector在stop和析构函数中注销该定时器
		retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs); // 重试的时间逐渐延长
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}