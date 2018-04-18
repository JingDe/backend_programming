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
	~TcpServer();  // Ϊunique_ptr��Աout-line??

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
	// ��acceptor_�����ӵ���ʱ�ص�,����TcpConnection���󣬼���ConnectionMap
	void newConnection(int sockfd, const InetAddress& peerAddr);
	void removeConnection(const TcpConnectionPtr& conn);

	typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

	EventLoop* loop_;  // // ��Acceptor::acceptChannel_��TcpConnection::channel_��ע�ᵽloop
	const std::string name_;
	std::unique_ptr<Acceptor> acceptor_; // ����accept��TCP����	
	ConnectionCallback connectionCallback_; // ���ӻص����������û����ã�TcpServer��ֵ��TcpConnection
	MessageCallback messageCallback_;	
	WriteCompleteCallback writeCompleteCallback_;
	bool started_;	
	int nextConnId_;// always in loop thread
	ConnectionMap connections_; // �����������ӣ�������-����
};

#endif
