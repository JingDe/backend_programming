#ifndef ACCEPTOR_H_
#define ACCEPTOR_H_

#include"socket.muduo/InetAddress.h"

#include"socket.muduo/Socket.h"
#include"reactor.muduo/Channel.h"
#include"reactor.muduo/EventLoop.h"

#include"common/noncopyable.h"


class Acceptor :public noncopyable{
public:
	typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort=true);
	~Acceptor();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{
		newConnectionCallback_ = cb;
	}

	bool listenning() const { return listenning_; }
	void listen();

private:
	void handleRead();

	EventLoop* loop_;
	Socket acceptSocket_;
	Channel acceptChannel_;
	bool listenning_;
	NewConnectionCallback newConnectionCallback_;
	int idleFd_; // ����EMFILE�����ļ��������ﵽ��������
	// ׼��һ�����е��ļ�����������EMFILEʱ���ȹر�idleFd_�����һ���ļ������������
	// Ȼ��accept�õ���socket���������������close���Ͽ��ͻ������ӡ����idleFd_���´�һ�������ļ�
};

#endif 
