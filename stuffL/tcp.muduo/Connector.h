#ifndef CONNECTOR_H_
#define CONNECTOR_H_

#include<memory>

#include"socket.muduo/InetAddress.h"
#include"reactor.muduo/Channel.h"

class EventLoop;

class Connector : public noncopyable, public std::enable_shared_from_this<Connector> {
public:
	typedef std::function<void(int)> NewConnectionCallback;

	Connector(EventLoop* loop, const InetAddress&);
	~Connector();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{
		newConnectionCallback_ = cb;
	}

	void start();  // can be called in any thread
	void restart();  // must be called in loop thread
	void stop();  // can be called in any thread

	const InetAddress& serverAddress() const { return serverAddr_; }

private:
	enum States { kDisconnected, kConnecting, kConnected };
	static const int kMaxRetryDelayMs = 30 * 1000;
	static const int kInitRetryDelayMs = 500;

	void setState(States s) { state_ = s; }
	void startInLoop();
	void stopInLoop();
	void connect();
	void connecting(int sockfd);
	void handleWrite();
	void handleError();
	void retry(int sockfd);
	void removeAndResetChannel();
	void resetChannel();

	EventLoop * loop_;
	InetAddress serverAddr_;	
	bool connect_; // atomic
	States state_;  // FIXME: use atomic variable
	int sockfd_; // 作为客户端，不需Socket的复杂功能
	std::unique_ptr<Channel> channel_; // sockfd_和channel_是一次性的
	NewConnectionCallback newConnectionCallback_;
	int retryDelayMs_;
};

typedef std::shared_ptr<Connector> ConnectorPtr;

#endif

