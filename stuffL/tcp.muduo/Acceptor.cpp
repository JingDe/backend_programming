#include"Acceptor.h"
#include"socket.muduo/SocketsOps.h"

#include<unistd.h>
#include<fcntl.h>

#include"logging.muduo/Logging.h"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort)
	:loop_(loop),
	acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
	acceptChannel_(loop, acceptSocket_.fd()),
	listenning_(false),
	idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
	acceptSocket_.setReuseAddr(true);
	acceptSocket_.setReusePort(reusePort);
	acceptSocket_.bindAddress(listenAddr);
	acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
	acceptChannel_.disableAll();
	acceptChannel_.remove();
	
}

void Acceptor::listen()
{
	loop_->assertInLoopThread();
	listenning_ = true;
	acceptSocket_.listen();
	acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
	loop_->assertInLoopThread();
	InetAddress peerAddr;
	int connfd = acceptSocket_.accept(&peerAddr);
	if (connfd >= 0)
	{
		if (newConnectionCallback_)
			newConnectionCallback_(connfd, peerAddr);
		else
			sockets::close(connfd);
	}
	else
	{
		LOG_ERROR << "in Acceptor::handleRead";

		if (errno == EMFILE) // 打开文件太多
		{
			::close(idleFd_);
			idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
			::close(idleFd_);
			idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC); // fcntl.h
		}
	}
}

