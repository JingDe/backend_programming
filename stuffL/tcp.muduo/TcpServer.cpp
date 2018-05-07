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

// ����TcpConnection���󣬼���ConnectionMap�����úûص�������TcpConnection::connectionEstablished()
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
	// ���߳�TcpServerʹ��acceptor loopͬʱ�۲������ӵ�IO�¼�
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
	//conn->connectEstablished();
	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

// removeConnection����TcpConnection��ioLoop�̵߳��ã���TcpServer�������ģ�������Ҫ��removeConnection
// �Ƶ�loop_�̵߳���
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// ��TcpConnection::connectDestroyed�Ƶ�TcpConnection��ioLoop�̵߳��ã�
// ��Ϊ�˱�֤TcpConnection��connectionCallback_ʼ������ioLoop�ص�������ͻ��˴���(connectionCallback_)�ı�д
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnection [" << name_ << "] - connection " << conn->name();
	size_t n = connections_.erase(conn->name()); // conn�����ü�����Ϊ1
	assert(n == 1);
	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		// std::bind��TcpConnection conn�������ڳ�������connectDestroyed��ʱ��
		// queueInLoop:�����ӶϿ�ʱ��ʵ���߳��л�
}