
// 客户端
// 连接服务器，发送两个数据，主动关闭连接

// nc ip port

// sizeof返回字节数

#include<stdio.h>
#include<string.h>

#include<unistd.h>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>

#include"threadmodel.h"

#define PORT 1880
#define IP "127.0.0.1"

void sendmsg(int sockfd)
{
	const char *msg[3]={"hello, server", "this is client", "i am sending 3 msgs"};

	for(int i=0; i<3; i++)
	{
		send(sockfd, msg[i], sizeof(msg[i]), 0);
	}
}

void client()
{
	struct epoll_event evts[10];
	bool ok=false;
	bool connected=false;
	
	int sockfd=socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
	{
		printf("socket() failed: %s\n", strerror(errno));
		return;
	}
	
	setNonBlocking(sockfd);
	
	struct sockaddr_in servAddr;
	servAddr.sin_family=AF_INET;
	servAddr.sin_port=htons(PORT);
	servAddr.sin_addr.s_addr=inet_addr(IP);
	
	int rc=connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr));
	int epfd=epoll_create(10);
	
	if(rc<0)
	{
		if(errno!=EINPROGRESS  &&  errno!=EWOULDBLOCK)
		{
			printf("connect() failed: %s\n", strerror(errno));
			close(sockfd);
			return ;
		}
		printf("wait connect\n");
		addfd(epfd, sockfd, EPOLLIN);
	}
	else if(rc==0)
	{
		printf("connected\n");
		connected=true;
	}
	
		
	printf("sockfd = %d\n", sockfd);
	
	char hello[20]="hello from client";
	while(!ok)
	{
		int tmout=5*1000;// 5秒钟
		int nEvents=epoll_wait(epfd, evts, sizeof(evts), tmout);
		if(nEvents<0)
		{
			printf("epoll_wait failed: %s\n", strerror(errno));
			break;
		}
		else if(nEvents==0)
		{
			printf("timeout\n");
			//if(connected)
			//	send(sockfd, hello, sizeof(hello), 0);
			continue;
		}
		
		printf("%d happened\n", nEvents);
		for(int i=0; i<nEvents; i++)
		{
			int fd=evts[i].data.fd;
			int evt=evts[i].events;
			
			printf("NO.%d event = %d\n", i, fd);
			
			if(fd==sockfd  &&  (evt & EPOLLIN))// 连接建立或数据到达
			{
				printf("EPOLLIN\n");
				if(connected==false)
				{
					int error = 0;
					socklen_t length = sizeof( error );
					if( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, &error, &length ) < 0 )
					{
						printf( "getsockopt failed: %s\n", strerror(errno));						
						break;
					}
					if(error==0)//错误码为0，连接成功建立
					{
						printf("connect ok\n");
						//addfd(epfd, sockfd, EPOLLOUT);
						modfd(epfd, sockfd, EPOLLIN  |  EPOLLOUT);
						
						
						send(sockfd, hello, sizeof(hello), 0);
						
						connected=true;
					}
					else
					{
						printf("connect failed: %s\n", strerror(errno));
						break;
					}
				}
				else// 接受数据
				{
					char buf[125];
					int n=recv(sockfd, buf, sizeof(buf), 0);
					if(n<0)
					{
						printf("recv failed: %s\n", strerror(errno));
						break;
					}
					else if(n==0)
					{
						break;
					}
					else
					{
						printf("RECV %s\n", buf);
					}
				}				
			}
			if(fd==sockfd  &&  (evt & EPOLLOUT))
			{
				printf("EPOLLOUT\n");
				sendmsg(sockfd);
				printf("CLIENT sendmsg ok\n");
				ok=true;
			}
			
			if(fd!=sockfd  ||  (!(evt & EPOLLIN)  &&  !(evt & EPOLLOUT)) )
			{
				printf("something else happened\n");
			}
		}
		
	}
	
	close(sockfd);
	close(epfd);
}

int main()
{
	client();
	
	
	return 0;
}