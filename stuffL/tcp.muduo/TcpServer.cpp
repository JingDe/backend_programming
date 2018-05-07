#include"TcpServer.h"
#include"TcpConnection.h"

#include<functional>

#include"reactor.muduo/EventLoopThreadPool.h"
#include"logging.muduo/Logging.h"

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
	: loop_(CHECK_NOTNULL(loop)),
	name_(listenAddr.toIpPort()),
	ipPort_(listenAddr.toIpPort()),
	acceptor_(new Acceptor(loop, listenAddr)),
	threadPool_(new EventLoopThreadPool(loop)),
	nextConnId_(1)
{
	acceptor_->setNewConnectionCallback(
		std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::setThreadNum(int numThreads)
{
	assert(0 <= numThreads);
	threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
	if (started_.getAndSet(1) == 0)
	{
		threadPool_->start(threadInitCallback_);

		assert(!acceptor_->listenning());
		loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
	}
}

// 创建TcpConnection对象，加入ConnectionMap，设置好回调，调用TcpConnection::connectionEstablished()
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	loop_->assertInLoopThread();
	EventLoop* ioLoop = threadPool_->getNextLoop();
	char buf[64];
	snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;

	LOG_INFO << "TcpServer::newConnection [" << name_
		<< "] - new connection [" << connName
		<< "] from " << peerAddr.toIpPort();
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	
	TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
	// 单线程TcpServer使用acceptor loop同时观察新连接的IO事件
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
	//conn->connectEstablished();
	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

// removeConnection会在TcpConnection的ioLoop线程调用，而TcpServer是无锁的，所以需要把removeConnection
// 移到loop_线程调用
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// 把TcpConnection::connectDestroyed移到TcpConnection的ioLoop线程调用，
// 是为了保证TcpConnection的connectionCallback_始终在其ioLoop回调，方便客户端代码(connectionCallback_)的编写
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnection [" << name_ << "] - connection " << conn->name();
	size_t n = connections_.erase(conn->name()); // conn的引用计数降为1
	assert(n == 1);
	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		// std::bind让TcpConnection conn的生命期长到调用connectDestroyed的时刻
		// queueInLoop:在连接断开时，实现线程切换
}