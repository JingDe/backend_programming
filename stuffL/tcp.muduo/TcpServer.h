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
	~TcpServer();  // Ϊunique_ptr��Աout-line??

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
	// ��acceptor_�����ӵ���ʱ�ص�,����TcpConnection���󣬼���ConnectionMap
	void newConnection(int sockfd, const InetAddress& peerAddr);
	void removeConnection(const TcpConnectionPtr& conn);
	void removeConnectionInLoop(const TcpConnectionPtr& conn);

	typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

	EventLoop* loop_;  // ���߳�TcpServer��Acceptor::acceptChannel_��TcpConnection::channel_��ע�ᵽloop
						// ���̵߳�accept loop
	const std::string name_;
	const std::string ipPort_;
	std::unique_ptr<Acceptor> acceptor_; // ����accept��TCP����	
	std::unique_ptr<EventLoopThreadPool> threadPool_; // ���߳�TcpServer���Լ���loop���ڽ��������ӣ�������ʹ������EventLoop
	ConnectionCallback connectionCallback_; // ���ӻص����������û����ã�TcpServer��ֵ��TcpConnection
	MessageCallback messageCallback_;	
	WriteCompleteCallback writeCompleteCallback_;

	ThreadInitCallback threadInitCallback_;
	// bool started_; // 
	AtomicInt32 started_; // ��ǵ�һ�ε���start
	int nextConnId_;// ������ţ���loop�߳�
	ConnectionMap connections_; // �����������ӣ�������-����
};

#endif
