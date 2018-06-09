#include<stdio.h>
#include<stdlib.h>// abort()

#include<sys/types.h>
#include<errno.h>
#include<sys/socket.h>
#include<string.h>
#include<fcntl.h>
#include<sys/epoll.h>


int CHECK(int state, const char* msg)
{
	if(state)
	{
		printf("%s failed: %s\n", msg, strerror(errno));
		abort();
	}
	return state;
}

void setReuseAddr(int fd)
{
	// 强制使用处于TIME_WAIT状态的连接占用的地址
	int reuse=1;
	CHECK(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)), "setsockopt");
}

int setNonBlocking(int fd)
{
	int oldoption=fcntl(fd, F_GETFL);
	int newoption =oldoption | O_NONBLOCK;
	fcntl(fd, F_SETFL, newoption);
	return oldoption;
}

void addfd(int epfd, int fd)
{
	struct epoll_event evt;
	evt.events=EPOLLIN;
	evt.data.fd=fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evt);
	
	setNonBlocking(fd);
}