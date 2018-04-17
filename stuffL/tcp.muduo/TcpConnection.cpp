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
	socket_(new Socket(sockfd)), // ����socket
	channel_(new Channel(loop, sockfd)), // �۲��������ϵ�IO�¼�
	localAddr_(localAddr),
	peerAddr_(peerAddr)
{
	LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd=" << sockfd;
	channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
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

// ������readable�¼�
void TcpConnection::handleRead()
{
	char buf[65535];
	ssize_t n = read(channel_->fd(), buf, sizeof buf); // socket_.fd()
	if (n > 0)
		messageCallback_(shared_from_this(), buf, n);
	else if (n == 0)
		handleClose();
	else
		handleError();
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