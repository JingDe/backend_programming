#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include<map>
#include<string>
#include<memory>

#include"common/noncopyable.h"
#include"tcp.muduo/Callbacks.h"
#include"Acceptor.h"
#include"common/Atomic.h"

class EventLoop;


class TcpServer : public noncopyable
{
public:
	TcpServer(EventLoop* loop, const InetAddress& listenAddr);
	~TcpServer();  // 为unique_ptr成员out-line??

	void start();

	void setConnectionCallback(const ConnectionCallback& cb)
	{
		connectionCallback_ = cb;
	}

	void setMessageCallback(const MessageCallback& cb)
	{ 
		messageCallback_ = cb;
	}

	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{
		writeCompleteCallback_ = cb;
	}

private:
	// 供acceptor_在连接到达时回调,创建TcpConnection对象，加入ConnectionMap
	void newConnection(int sockfd, const InetAddress& peerAddr);
	void removeConnection(const TcpConnectionPtr& conn);

	typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

	EventLoop* loop_;  // // 将Acceptor::acceptChannel_和TcpConnection::channel_都注册到loop
	const std::string name_;
	std::unique_ptr<Acceptor> acceptor_; // 用于accept新TCP连接	
	ConnectionCallback connectionCallback_; // 连接回调函数，由用户设置，TcpServer赋值给TcpConnection
	MessageCallback messageCallback_;	
	WriteCompleteCallback writeCompleteCallback_;
	bool started_;	
	int nextConnId_;// always in loop thread
	ConnectionMap connections_; // 管理所有连接，连接名-连接
};

#endif
