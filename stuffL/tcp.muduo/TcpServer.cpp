#include"TcpServer.h"
#include"TcpConnection.h"

#include<functional>

#include"logging.muduo/Logging.h"

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
	: loop_(CHECK_NOTNULL(loop)),
	name_(listenAddr.toIpPort()),
	acceptor_(new Acceptor(loop, listenAddr)),
	started_(false),
	nextConnId_(1)
{
	acceptor_->setNewConnectionCallback(
		std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
	if (!started_)
	{
		started_ = true;
	}

	if (!acceptor_->listenning())
	{
		loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
	}
}

// ����TcpConnection���󣬼���ConnectionMap�����úûص�������TcpConnection::connectionEstablished()
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	loop_->assertInLoopThread();
	char buf[32];
	snprintf(buf, sizeof buf, "#%d", nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;

	LOG_INFO << "TcpServer::newConnection [" << name_
		<< "] - new connection [" << connName
		<< "] from " << peerAddr.toIpPort();
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	
	TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
	// ʹ��acceptor loopͬʱ�۲������ӵ�IO�¼�
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
	conn->connectEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnection [" << name_ << "] - connection " << conn->name();
	size_t n = connections_.erase(conn->name()); // conn�����ü�����Ϊ1
	assert(n == 1);
	loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		// std::bind��TcpConnection�������ڳ�������connectDestroyed��ʱ��
		// queueInLoop:
}