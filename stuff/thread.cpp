

//

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
	//printf("thread->pipefd = %d\n", thread->pipefd);

	
	int epfd=epoll_create(50);
	
	//printf("epfd = %d\n", epfd);

	addfd(epfd, thread->pipefd);
	struct epoll_event evts[50];
	
	while(true)
	{
		int nEvents=epoll_wait(epfd, evts, sizeof(evts), -1);
		if(nEvents<0  &&  errno!=EINTR)
		{
			break;
		}
		
		for(int i=0; i<nEvents; i++)
		{
			int fd=evts[i].data.fd;
			int event=evts[i].events;
			if(fd==thread->pipefd)
			{
				char buf[10];
				int nrecv=recv(thread->pipefd, buf, sizeof buf, 0);
				if(nrecv<0)
				{
					printf("recv(pipefd) error: %s\n", strerror(errno));
					break;
				}
				thread->connfd=atoi(buf);
				
				addfd(epfd, thread->connfd);
				thread->busy=true;
			}
			else if(thread->isbusy()==true  &&  fd==thread->connfd  &&  event & EPOLLIN)
			{
				// 处理客户请求，使用计算线程池完成计算，
				char buf[512];
				int nrecv=recv(thread->connfd, buf, sizeof buf, 0);
				if(nrecv<0)
				{
					printf("recv(connfd) failed: %s\n", strerror(errno));
					close(thread->connfd);
					thread->busy=false;
				}
				else if(nrecv==0)
				{
					printf("client disconnect\n");
					close(thread->connfd);
					thread->busy=false;
				}
				
				// 将请求交给计算线程处理
			}
			else if(thread->isbusy()==true  &&  fd==thread->connfd  &&  event & EPOLLOUT)
			{
				//并将计算结果发送给客户连接
				snprintf(thread->res, sizeof(thread->res), "fake result");
				int nsend=send(thread->connfd, thread->res, strlen(thread->res), 0);
				if(nsend<0)
				{
					printf("send(connfd) failed: %s\n", strerror(errno));
				}
			}
			else
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
{	
	//Thread *thread=(Thread*)arg;
	(void)arg;
	
	int epfd=epoll_create(50);
	addfd(epfd, pipefd);
	struct epoll_event evts[50];
	
	while(true)
	{
		int nEvents=epoll_wait(epfd, evts, sizeof(evts), -1);
		if(nEvents<0  &&  errno!=EINTR)
		{
			break;
		}
		
		for(int i=0; i<nEvents; i++)
		{
			int fd=evts[i].data.fd;
			int event=evts[i].events;
			if(fd==pipefd)
			{
				char buf[10];
				int nrecv=recv(pipefd, buf, sizeof buf, 0);
				if(nrecv<0)
				{
					printf("recv(pipefd) error: %s\n", strerror(errno));
					break;
				}
				connfd=atoi(buf);
				
				addfd(epfd, connfd);
				busy=true;
			}
			else if(busy==true  &&  fd==connfd  &&  event & EPOLLIN)
			{
				// 处理客户请求，使用计算线程池完成计算，
				char buf[512];
				int nrecv=recv(connfd, buf, sizeof buf, 0);
				if(nrecv<0)
				{
					printf("recv(connfd) failed: %s\n", strerror(errno));
					close(connfd);
					busy=false;
				}
				else if(nrecv==0)
				{
					printf("client disconnect\n");
					close(connfd);
					busy=false;
				}
				
				// 将请求交给计算线程处理
			}
			else if(busy==true  &&  fd==connfd  &&  event & EPOLLOUT)
			{
				//并将计算结果发送给客户连接
				snprintf(res, sizeof(res), "fake result");
				int nsend=send(connfd, res, strlen(res), 0);
				if(nsend<0)
				{
					printf("send(connfd) failed: %s\n", strerror(errno));
				}
			}
			else
			{
				printf("other events happend\n");
			}
		}
	}
	
	// 通知主线程
	
	return (void*)0;
}*/