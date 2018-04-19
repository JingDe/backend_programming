#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include<memory>
#include<string>

#include"common/MutexLock.h"
#include"Callbacks.h"
#include"common/noncopyable.h"

class EventLoop;
class Connector;
class InetAddress;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable
{
public:
	TcpClient(EventLoop* loop,
		const InetAddress& serverAddr,
		const std::string& nameArg);
	~TcpClient();  // force out-line dtor, for scoped_ptr members.

	void connect();
	void disconnect();
	void stop();

	TcpConnectionPtr connection() const
	{
		MutexLock lock(mutex_);
		return connection_;
	}

	EventLoop* getLoop() const { return loop_; }
	bool retry() const { return retry_; }
	void enableRetry() { retry_ = true; }

	const std::string& name() const
	{
		return name_;
	}

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
	void newConnection(int sockfd);
	void removeConnection(const TcpConnectionPtr& conn);

	EventLoop* loop_;
	ConnectorPtr connector_; // 
	const std::string name_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	bool retry_;   // atomic
	bool connect_; // atomic

	int nextConnId_;
	mutable port::Mutex mutex_;
	TcpConnectionPtr connection_;
};



#endif  
