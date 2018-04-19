#ifndef TCPCONNECTION_H_
#define TCPCONNECTION_H_

#include<memory>
#include<string>

#include"Callbacks.h"
#include"socket.muduo/InetAddress.h"
#include"Buffer.h"

class EventLoop;
class Socket;
class Channel;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> 
{
public:
	TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
	~TcpConnection();

	EventLoop* getLoop() const { return loop_; }
	const std::string& name() const { return name_; }
	const InetAddress& localAddress() { return localAddr_; }
	const InetAddress& peerAddress() { return peerAddr_; }
	bool connected() const { return state_ == kConnected; }

	void send(const std::string& message);
	void shutdown();
	void forceClose();

	void setTcpNoDelay(bool on); // 禁用Nagle算法
	void setKeepAlive(bool on);

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

	void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
	{
		highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark;
	}

	void setCloseCallback(const CloseCallback& cb) {
		closeCallback_ = cb;
	}

	void connectEstablished(); // 当TcpServer::newConnection调用
	void connectDestroyed(); // 当TcpServer删除本连接调用

private:
	enum StateE{ kConnecting, kConnected, kDisconnecting, kDisconnected, };

	void setState(StateE s) { state_ = s; }

	void handleRead(Timestamp receiveTime); // 供channel_在readable事件时回调
	void handleWrite();
	void handleClose();
	void handleError();

	void sendInLoop(const std::string& message);
	void shutdownInLoop();
	void forceCloseInLoop();

	EventLoop* loop_;
	std::string name_; // 连接名称
	StateE state_; // 连接状态
	std::unique_ptr<Socket> socket_; // 拥有连接的socket，负责close
	std::unique_ptr<Channel> channel_; // 管理socket_的IO事件
	InetAddress localAddr_;
	InetAddress peerAddr_;
	ConnectionCallback connectionCallback_; // connectEstablished()和connectDestroyed()均调用
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	HighWaterMarkCallback highWaterMarkCallback_;
	CloseCallback closeCallback_;
	size_t highWaterMark_;
	Buffer inputBuffer_;
	Buffer outputBuffer_;
};

#endif