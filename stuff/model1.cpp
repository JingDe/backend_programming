
// 多线程模型
// reactors in threads

// socketpair，全双工管道，套接字管道。
// AF_UNIX/AF_LOCAL, pipefd[2]
// 父子进程通信

// g++ model1.cpp thread.cpp threadmodel.cpp -o model1.out -pthread -g -Wall
// -std=c++11

// 不能捕获SIGKILL SIGSTOP
// ctrl+z  SIGHUP  挂起
// ctrl+c  SIGINT  终止当前程序
// kill    SIGTERM  终止
// ctrl+\  SIGTERM  终止所有前台进程组

#include"thread.h"
#include"threadmodel.h"

#include<stdio.h>
#include<stdlib.h>

#include<unistd.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<signal.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<string.h>

#define PORT 1880
#define IP "127.0.0.1"
const static int MAXQ=100;
const static int WORKERS=8;

int sigfd[2];


void sighandler(int signo)
{
	printf("SIGNO %d\n", signo);
	int save_errno=errno;
	int msg=signo;
	char tmp[10];
	//int n=snprintf(tmp, sizeof(tmp), "%d", msg);
	write(sigfd[1], (char*)&msg, 1);
	//write(sigfd[1], tmp, strlen(tmp));
	errno=save_errno;
}

void main_reactor()
{
	struct epoll_event evts[10];
	int listenFd;
	int epfd;
	
	listenFd=socket(AF_INET, SOCK_STREAM, 0);
	
	setReuseAddr(listenFd);
	//setNonBlocking(listenFd);
	
	struct sockaddr_in serverAddr;
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_port=htons(PORT);
	serverAddr.sin_addr.s_addr=inet_addr(IP);
	// inet_aton(IP, &serverAddr.sin_addr);
	CHECK(bind(listenFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)), "bind");
	CHECK(listen(listenFd, MAXQ), "listen");
	
	epfd=epoll_create(1);
	
	addfd(epfd, listenFd, EPOLLIN);
	
	// 统一事件源，添加sigfd
	// TODO: sigaction
	CHECK(pipe(sigfd), "pipe");
	signal(SIGHUP, sighandler);//挂起终端
	signal(SIGPIPE, sighandler);//往读端关闭的管道或者socket写数据
	signal(SIGURG, sighandler);//带外数据到达
	//signal(SIGTERM, sighandler);//终止进程
	//signal(SIGINT, sighandler);//Ctrl+C
	
	setNonBlocking(sigfd[1]);
	addfd(epfd, sigfd[0], EPOLLIN);
	
	// 计算线程池
	
	
	// 创建子线程池
	// TODO: ThreadPool workers(WORKERS);
	Thread workers[WORKERS];
	int pipes[WORKERS][2];
	for(int i=0; i<WORKERS; i++)
	{
		//CHECK(pthread_create(&(workers[i].tid), NULL, work, NULL), "pthread_create");
		socketpair(AF_UNIX, SOCK_STREAM, 0, pipes[i]);
		//workers[i].pipefd=pipes[i][1];
		//workers[i].busy=false;
		//workers[i].work=work;
		workers[i].setNo(i);
		workers[i].setPipefd(pipes[i][1]);
		//printf("pipes[%d][1]=%d\n", i, pipes[i][1]);
		workers[i].run();
		
		addfd(epfd, pipes[i][0], EPOLLIN);// 主线程通过pipes[i][0]与第i个工作线程的pipes[i][1]通信
	}
	
		
	while(true)
	{
		int nEvents=epoll_wait(epfd, evts, sizeof(evts), -1);
		if(nEvents<0  &&  errno!=EINTR)
		{
			printf("epoll_wait failed: %s\n", strerror(EINTR));
			break;
		}
		
		printf("%d events happened\n", nEvents);
		for(int i=0; i<nEvents; i++)
		{
			int fd=evts[i].data.fd;
			if(fd==listenFd)
			{
				struct sockaddr_in cliAddr;
				socklen_t socklen=sizeof(cliAddr);
				int connfd=accept(listenFd, (struct sockaddr*)&cliAddr, &socklen);
				if(connfd<0)
				{
					printf("accept failed: %s\n", strerror(errno));
					break;
				}
				else
				{
					printf("get client : %d\n", connfd);
				}
				char hello[20]="hello from server";
				send(connfd, hello, sizeof(hello), 0);
				
				// 将连接挂到子reactor上
				int x;
				for(x=0; x<WORKERS; x++)
				{
					//printf("worker %d is %d\n", x, workers[x].isbusy());
					if(workers[x].isbusy()==false)
					{
						printf("FOUND worker %d is free=%d\n", x, workers[x].isbusy());
						break;
					}
				}
				
				if(x<WORKERS  &&  workers[x].isbusy()==false)
				{
					printf("assign to worker%d\n", x);
					//printf("MAIN %s, %d\n", (char*)&connfd, sizeof(connfd));
					//send(pipes[x][0], (char*)&connfd, sizeof(connfd), 0);
					char temp[10];
					int n=snprintf(temp, sizeof temp, "%d", connfd);
					//printf("TEMP %s, %d\n", temp, n);
					send(pipes[x][0], temp, n, 0);
				}
				else
				{
					printf("server is busy\n");
					close(connfd);
				}
			}
			else if(fd==sigfd[0])
			{
				printf("sigfd[0]=%d\n", sigfd[0]);
				char sigbuf[10]={0};
				int nread=read(sigfd[0], sigbuf, sizeof(sigbuf));
				if(nread>0)
				{
					printf("SIGBUF %s, %d\n", sigbuf, nread);
				}
				for(int i=0; i<nread; i++)
				{
					//int signo=atoi(reinterpret_cast<const char*>(sigbuf[i]));
					//switch(signo)
					switch(sigbuf[i])
					{
						case SIGHUP:
							printf("SIGHUP\n");
							break;
						/*case SIGINT:
							printf("SIGINT\n");
							break;
						case SIGTERM:
							printf("SIGTERM\n");
							break;*/
						case SIGPIPE:
							printf("SIGPIPE\n");
							break;
						case SIGURG:
							printf("SIGURG\n");
							break;
					}
				}
			}
			else
			{
				printf("other events happend: %d\n", fd);
			}
		}
	}
	
	close(listenFd);
	
	
}

int main()
{
	main_reactor();
	
	return 0;
}