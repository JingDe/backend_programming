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
	assert(state_ == kConnected || state_ == kDisconnecting);
	channel_->disableAll();
	closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
	int err = sockets::getSocketError(channel_->fd());
	LOG_ERROR << "TcpConnection::handleError[" << name_ << "] - SO_ERROR = " << err << " " << strerror(err);
}

void TcpConnection::handleWrite()
{
	loop_->assertInLoopThread();
	if (channel_->isWriting())
	{
		ssize_t n = write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
		if (n > 0)
		{
			outputBuffer_.retrieve(n);
			if (outputBuffer_.readableBytes() == 0)
			{
				channel_->disableWriting(); // 避免busy loop
				if (writeCompleteCallback_)
				{
					loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
				}
				if (state_ == kDisconnecting) // 继续关闭连接
					shutdownInLoop(); // 已在IO线程
			}
			else
			{
				LOG_TRACE << "I am going to write more data";
			}
		}
		else
		{
			LOG_ERROR << "TcpConnection::handleWrite";
		}
	}
	else
	{
		LOG_TRACE << "Connection is down, no more writing";
	}
}

void TcpConnection::connectDestroyed()
{
	loop_->assertInLoopThread();
	assert(state_ == kConnected  ||  state_==kDisconnecting);
	setState(kDisconnected);
	channel_->disableAll();

	connectionCallback_(shared_from_this());

	//loop_->removeChannel(channel_.get());
	channel_->remove();// 间接调用loop_->removeChannel()
}

void TcpConnection::send(const std::string& message)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
			sendInLoop(message);
		else
			loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
		// 将message拷贝一份，传给IO线程中的sendInLoop来发送
	}
}

void TcpConnection::sendInLoop(const std::string& message)
{
	loop_->assertInLoopThread();
	ssize_t nwrote = 0;
	size_t remaining = message.size();
	if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
	{
		nwrote = write(channel_->fd(), message.data(), message.size());
		if (nwrote >= 0)
		{
			remaining = remaining - nwrote;
			if (static_cast<size_t>(nwrote) < message.size())
			{
				LOG_TRACE << "I am going to write more data";
			}	
			else if(writeCompleteCallback_)
			{
				loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
			}
		}
		else
		{
			nwrote = 0;
			if (errno != EWOULDBLOCK)
				LOG_ERROR << "TcpConnection::sendInLoop";
		}
	}

	assert(nwrote >= 0);
	if (static_cast<size_t>(nwrote) < message.size())
	{
		size_t oldLen = outputBuffer_.readableBytes();
		if (oldLen + remaining >= highWaterMark_
			&& oldLen < highWaterMark_
			&& highWaterMarkCallback_)
		{
			loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
		}
		outputBuffer_.append(message.data() + nwrote, message.size() - nwrote); // remaining
		if (!channel_->isWriting())
			channel_->enableWriting();
	}
}

void TcpConnection::shutdown()
{
	if (state_ == kConnected)
	{
		setState(kDisconnecting);
		loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
	}
}

void TcpConnection::shutdownInLoop()
{
	loop_->assertInLoopThread();
	if (!channel_->isWriting())
		socket_->shutdownWrite();
}

void TcpConnection::setTcpNoDelay(bool on)
{
	socket_->setTcpNoDelay(on);
}

void TcpConnection::setKeepAlive(bool on)
{
	socket_->setKeepAlive(on);
}