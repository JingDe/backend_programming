
// 客户端
// 连接服务器，发送两个数据，主动关闭连接

// nc ip port

// sizeof返回字节数

// 只在需要发送数据的时候关注EPOLLOUT事件

// 非阻塞connect的方法：
// 调用connect若返回EINPROGRESS，关注sockfd的OUT事件，当EPOLLOUT时，
// 调用getsockopt获取和清除错误，若错误码为0，连接成功建立，否则出错
// 移植性问题：
// 连接建立错误时，既可读又可写
// 判断连接成功？
// (1) 调用getpeername
// (2) 以长度为0调用read
// (3) 再调用一次connect
// (4) 
// 

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


bool connected1(int sockfd)
{
	int error = 0;
	socklen_t length = sizeof( error );
	if( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, &error, &length ) < 0 )
	{
		printf( "getsockopt failed: %s\n", strerror(errno));						
		return false;
	}
	if(error==0)//错误码为0，连接成功建立
	{
		return true;
	}
	else
		return false;
}

bool connected2(int sockfd)
{
	char buf[10];
	int nread=read(sockfd, buf, 0);
	if(read==0)
		return true;
	else
	{
		printf("connect failed: %d\n", strerror(errno));
		return false;
	}
}


// TODO: 应用层buffer
void sendreq(int sockfd)
{
	const char msg[]="this is remaining requests";

	int nsend=send(sockfd, msg, strlen(msg), 0);
	(void)nsend;
}

void client()
{
	struct epoll_event evts[10];
	bool ok=false;
	bool connected=false;
	char hello[]="hello from client";
	char buf[125];
	
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
		if(errno!=EINPROGRESS)
		{
			printf("connect() failed: %s\n", strerror(errno));
			close(sockfd);
			return ;
		}

		addfd(epfd, sockfd, EPOLLIN);
	}
	else if(rc==0)
	{
		printf("connected\n");
		connected=true;
	}
	
	
	while(!ok)
	{
		int tmout=5*1000;// 5秒钟
		int nEvents=epoll_wait(epfd, evts, sizeof(evts), tmout);
		if(nEvents<0)
		{
			printf("epoll_wait failed: %s\n", strerror(errno));
			break;
		}

		
		printf("%d happened\n", nEvents);
		for(int i=0; i<nEvents; i++)
		{
			int fd=evts[i].data.fd;
			int evt=evts[i].events;
			
			printf("NO.%d event = %d\n", i, fd);
			
			if(fd==sockfd)
			{
				if(evt & EPOLLIN)// 连接建立或数据到达
				{
					printf("EPOLLIN\n");
					if(connected==false)
					{
						connected=connected1(sockfd);
						if(connected)
						{
							printf("connected\n");							
							
							//send(sockfd, hello, sizeof(hello), 0);
							//rc=sendreq(sockfd);						
							
							const char msg[]="this is client requests";
							int nsend=send(sockfd, msg, strlen(msg), 0);
							if(nsend<0)
							{
								printf("send failed: %s\n", strerror(errno));
								modfd(epfd, sockfd, EPOLLOUT);// return??
							}
							else if(nsend<strlen(msg))
								modfd(epfd, sockfd, EPOLLOUT);
														
						}
						else
						{
							printf("connect failed: %s\n", strerror(errno));
							close(sockfd);
							close(epfd);
							return;
						}
					}
					else// 接受数据
					{						
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
							buf[n]=0;
							printf("RECV %s\n", buf);
							if(memcmp(buf, "fake result", n)==0)
							{
								printf("recv result ok\n");
								// delmd(epfd, sockfd, EPOLLIN);
								ok=true;
							}
						}
					}				
				}
				else if(evt & EPOLLOUT)
				{
					// TODO : 发送buffer剩余数据，继续关注OUT
					printf("EPOLLOUT\n");
					sendreq(sockfd);					
					modfd(epfd, sockfd, EPOLLIN);
				}
			}					
			else//if(fd!=sockfd  ||  (!(evt & EPOLLIN)  &&  !(evt & EPOLLOUT)) )
			{
				printf("something else happened\n");
			}
		}
		
	}
	printf("exit while\n");
	
	close(sockfd);
	close(epfd);
}

int main()
{
	client();
	
	
	return 0;
}