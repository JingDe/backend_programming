#include"reactor.muduo/EventLoop.h"
#include"socket.muduo/InetAddress.h"
#include"tcp.muduo/Acceptor.h"

#include<unistd.h>

void newConnection(int sockfd, const InetAddress& peerAddr)
{
	printf("newConnection(): accepted a new connection from %s\n", peerAddr.toIpPort().c_str());
	::write(sockfd, "how are you?\n", 13);
	sockets::close(sockfd);
}

int main()
{
	InetAddress listenAddr(9981);
	EventLoop loop;

	Acceptor acceptor(&loop, listenAddr);
	acceptor.setNewConnectionCallback(newConnection);
	acceptor.listen();

	loop.loop();
}