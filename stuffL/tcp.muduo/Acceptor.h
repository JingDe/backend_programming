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
	int idleFd_; // 处理EMFILE错误，文件描述符达到进程上线
	// 准备一个空闲的文件描述符。当EMFILE时，先关闭idleFd_，获得一个文件描述符的名额，
	// 然后accept拿到新socket的描述符，并理解close，断开客户端连接。最后idleFd_重新打开一个空闲文件
};

#endif 
