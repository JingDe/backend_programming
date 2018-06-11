

// 在写到EAGAIN时，才注册EPOLLOUT
// 为避免网络延迟带来的的数据发送接收延迟，异步设置EPOLLIN和EOLLOUT
// 错误示例：接收到数据，关注EPOLLOUT，接收到连接关闭，不关注EPOLLOUT，接收到延迟的数据，关注EPOLLOUT，出错

#include"thread.h"
#include"threadmodel.h"

#include<stdio.h>
#include<stdlib.h>
#include<functional>

#include<unistd.h>
#include<sys/epoll.h>
#include<errno.h>
#include<string.h>
#include<sys/socket.h>
#include<pthread.h>
#include<pthread.h>

#define MAXEVENTS 20
#define TIMEOUTMS 5*1000
#define BUFSZ 200

Thread::Thread()
	:busy(false)
{
	
}

Thread::~Thread()
{
	if(busy)
	{
		close(connfd);
	}
	busy=false;
	
}

void* work(void* arg);
void Thread::run()
{
	//setNonBlocking(pipefd);
	
	//pthread_create(&tid, NULL, std::bind(&Thread::work, this), NULL);// c++11
	//pthread_create(&tid, NULL, std::bind(&Thread::work, this, std::placeholders::_1), NULL);
	pthread_create(&tid, NULL, work, this);
}


void* work(void* arg)
{	
	Thread *thread=(Thread*)arg;
	//(void)arg;
	char buf[BUFSZ];
	int epfd=epoll_create(MAXEVENTS);

	addfd(epfd, thread->pipefd, EPOLLIN);
	struct epoll_event evts[MAXEVENTS];
	
	while(true)
	{
		int tmout=TIMEOUTMS;// 5秒钟
		int nEvents=epoll_wait(epfd, evts, sizeof(evts), tmout);
		// TODO: 断开不活跃的客户端连接
		
		if(nEvents<0  &&  errno!=EINTR)
		{
			break;
		}
		else if(nEvents==0)
		{
			printf("worker timeout, state is %d\n", thread->busy);
			continue;
		}
		
		for(int i=0; i<nEvents; i++)
		{
			int fd=evts[i].data.fd;
			int event=evts[i].events;
			
			
			if(fd==thread->pipefd)
			{				
				int nrecv=recv(thread->pipefd, buf, sizeof buf, 0);
				if(nrecv<=0)
				{
					printf("recv(pipefd) error: %s\n", strerror(errno));
					break;
				}								
				
				thread->connfd=atoi(buf);				
				
				addfd(epfd, thread->connfd, EPOLLIN);
				thread->busy=true;
				printf("worker %d handle %d\n", thread->no, thread->connfd);
				
				char hello[20]="hello from worker";
				send(thread->connfd, hello, sizeof(hello), 0);
				printf("send hello from worker ok\n");
				continue;
			}
			else if(thread->isbusy()==true  &&  fd==thread->connfd)
			{
				if(event & EPOLLIN)
				{
					// 处理客户请求，使用计算线程池完成计算，
					int nrecv=recv(thread->connfd, buf, sizeof buf, 0);
					if(nrecv<=0)
					{
						close(thread->connfd);
						thread->connfd=-1;
						thread->busy=false;
						
						delfd(epfd, thread->connfd, EPOLLIN);
					}
					else
					{
						// TODO:将请求交给计算线程处理
						buf[nrecv]=0;
						printf("RECV '%s'\n", buf);
						
						send(thread->connfd, buf, nrecv, 0);
						
						//addfd(epfd, thread->connfd, EPOLLOUT);
						modfd(epfd, thread->connfd, EPOLLIN  | EPOLLOUT);
					}					
				}
			
				if(event & EPOLLOUT)
				{
					//计算结果发送给客户连接
					snprintf(thread->res, sizeof(thread->res), "fake result");
					int nsend=send(thread->connfd, thread->res, strlen(thread->res), 0);
					if(nsend<0)
					{
						printf("send(connfd) failed: %s\n", strerror(errno));
						close(thread->connfd);
						thread->connfd=-1;
						delfd(epfd, thread->connfd, EPOLLOUT);
						break;
					}
					else if(nsend==strlen(thread->res))
						modfd(epfd, thread->connfd, EPOLLIN);
				}
			}
			else//if(fd!=thread->connfd  &&  fd!=thread->pipefd)
			{
				printf("other events happend\n");
			}
		}
	}
	// 通知主线程
	
	return (void*)0;
}

/*
void* Thread::work(void* arg)
}*/