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

	void setTcpNoDelay(bool on); // ����Nagle�㷨
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

	void connectEstablished(); // ��TcpServer::newConnection����
	void connectDestroyed(); // ��TcpServerɾ�������ӵ���

private:
	enum StateE{ kConnecting, kConnected, kDisconnecting, kDisconnected, };

	void setState(StateE s) { state_ = s; }

	void handleRead(Timestamp receiveTime); // ��channel_��readable�¼�ʱ�ص�
	void handleWrite();
	void handleClose();
	void handleError();

	void sendInLoop(const std::string& message);
	void shutdownInLoop();
	void forceCloseInLoop();

	EventLoop* loop_;
	std::string name_; // ��������
	StateE state_; // ����״̬
	std::unique_ptr<Socket> socket_; // ӵ�����ӵ�socket������close
	std::unique_ptr<Channel> channel_; // ����socket_��IO�¼�
	InetAddress localAddr_;
	InetAddress peerAddr_;
	ConnectionCallback connectionCallback_; // connectEstablished()��connectDestroyed()������
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	HighWaterMarkCallback highWaterMarkCallback_;
	CloseCallback closeCallback_;
	size_t highWaterMark_;
	Buffer inputBuffer_;
	Buffer outputBuffer_;
};

#endif