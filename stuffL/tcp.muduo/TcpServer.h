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
class EventLoopThreadPool;

class TcpServer : public noncopyable
{
public:
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	TcpServer(EventLoop* loop, const InetAddress& listenAddr);
	~TcpServer();  // 为unique_ptr成员out-line??

	void setThreadNum(int numThreads);
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{
		threadInitCallback_ = cb;
	}

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
	void removeConnectionInLoop(const TcpConnectionPtr& conn);

	typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

	EventLoop* loop_;  // 单线程TcpServer将Acceptor::acceptChannel_和TcpConnection::channel_都注册到loop
						// 多线程的accept loop
	const std::string name_;
	const std::string ipPort_;
	std::unique_ptr<Acceptor> acceptor_; // 用于accept新TCP连接	
	std::unique_ptr<EventLoopThreadPool> threadPool_; // 多线程TcpServer，自己的loop用于接受新连接，新连接使用其他EventLoop
	ConnectionCallback connectionCallback_; // 连接回调函数，由用户设置，TcpServer赋值给TcpConnection
	MessageCallback messageCallback_;	
	WriteCompleteCallback writeCompleteCallback_;

	ThreadInitCallback threadInitCallback_;
	// bool started_; // 
	AtomicInt32 started_; // 标记第一次调用start
	int nextConnId_;// 连接序号，在loop线程
	ConnectionMap connections_; // 管理所有连接，连接名-连接
};

#endif
