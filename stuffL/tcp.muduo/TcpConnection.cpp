#include"TcpConnection.h"
#include"socket.muduo/Socket.h"
#include"reactor.muduo/Channel.h"
#include"reactor.muduo/EventLoop.h"

#include<functional>

#include<unistd.h>
#include<errno.h>

#include"logging.muduo/Logging.h"

TcpConnection::TcpConnection(EventLoop* loop,
	const std::string& nameArg,
	int sockfd,
	const InetAddress& localAddr,
	const InetAddress& peerAddr)
	:loop_(CHECK_NOTNULL(loop)),
	name_(nameArg),
	state_(kConnecting),
	socket_(new Socket(sockfd)), // 连接socket
	channel_(new Channel(loop, sockfd)), // 观察新连接上的IO事件
	localAddr_(localAddr),
	peerAddr_(peerAddr)
{
	LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd=" << sockfd;
	// ?? handleRead无参数，ReadCallback有参数
	channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)); 
	channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
	channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
	LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at" << this << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished()
{
	loop_->assertInLoopThread();
	assert(state_ == kConnecting);
	setState(kConnected);
	channel_->enableReading();

	connectionCallback_(shared_from_this());
}

// 处理可readable事件
void TcpConnection::handleRead(Timestamp receiveTime)
{
	//char buf[65535];
	//ssize_t n = read(channel_->fd(), buf, sizeof buf); // socket_.fd()
	int savedErrno;
	ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
	if (n > 0)
		messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	else if (n == 0)
		handleClose();
	else
	{
		errno = savedErrno;
		handleError();
	}
}

void TcpConnection::handleClose()
{
	loop_->assertInLoopThread();
	LOG_TRACE << "TcpConnection::handleClose state=" << state_;
	assert(state_ == kConnected);
	channel_->disableAll();
	closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
	int err = sockets::getSocketError(channel_->fd());
	LOG_ERROR << "TcpConnection::handleError[" << name_ << "] - SO_ERROR = " << err << " " << strerror(err);
}

void TcpConnection::connectDestroyed()
{
	loop_->assertInLoopThread();
	assert(state_ == kConnected);
	setState(kDisconnected);
	channel_->disableAll();

	connectionCallback_(shared_from_this());

	//loop_->removeChannel(channel_.get());
	channel_->remove();// 间接调用loop_->removeChannel()
}