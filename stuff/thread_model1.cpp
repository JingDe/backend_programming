
// 多线程模型
// reactors in threads
#include<stdio.h>

#include<sys/types.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/epoll.h>

#define PORT 1880
#define IP "127.0.0.1"
const static int MAXQ=100;
const static int WORKERS=8;

void CHECK(int state, const char* msg)
{
	if(state)
	{
		printf("%s failed: %s\n", msg, strerror_s(errno));
		abort();
	}
}

void setReuseAddr(int fd)
{
	// 强制使用处于TIME_WAIT状态的连接占用的地址
	int reuse=1;
	CHECK(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reuse, sizeof(reuse)), "setsockopt");
}

int setNonBlocking(int fd)
{
	int oldoption=fcntl(fd, F_GETFL);
	int newoption =oldoption | O_NONBLOCKING;
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


void work(void)
{
	
}

class Thread{
private:
	pthread_t tid;
	int pipefd;
	
	bool busy;
	int connfd;
	void (*work)();
	
public:
	Thread();
	~Thread();
	
	void run();
};

void Thread::run()
{
	setNonblocking(pipefd);
	
	pthread_create(&tid, NULL, work, NULL);
	
}

void main_reactor()
{
	struct epoll_event evts[10];
	int listenFd;
	int epfd;
	
	listenFd=CHECK(socket(AF_INET, SOCK_STREAM, 0), "socket");
	
	setReuseAddr(listenFd);
	//setNonBlocking(listenFd);
	
	struct sockaddr_in4 serverAddr;
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_port=inet_pton(PORT);
	serverAddr.sin_addr.s_addr=inet_addr(IP);
	// inet_aton(IP, &serverAddr.sin_addr);
	CHECK(bind(listenFd, (struct socket_addr*)&serverAddr, sizeof(serverAddr)), "bind");
	CHECK(listen(listenFd, MAXQ), "listen");
	
	epfd=CHECK(epoll_create(1), "epoll_create");
	
	addfd(epfd, listenFd);
	
	
	// 创建子线程池
	// TODO: ThreadPool workers(WORKERS);
	Thread workers[WORKERS];
	int pipes[WORKERS][2];
	for(int i=0; i<WORKERS; i++)
	{
		//CHECK(pthread_create(&(workers[i].tid), NULL, work, NULL), "pthread_create");
		socketpair(AF_UNIX, SOCK_STREAM, 0, pipes[i]);
		workers[i].pipefd=pipes[i][1];
		workers[i].busy=false;
		workers[i].work=work;
		workers[i].run();
		
		addfd(epfd, pipes[i][0]);// 主线程通过pipes[i][0]与第i个工作线程的pipes[i][1]通信
	}
	
		
	while(true)
	{
		int nEvents=epoll_wait(epfd, evts, sizeof(evts), -1);
		if(nEvents<0  &&  errno!=EINTR)
		{
			printf("epoll_wait failed: %s\n", strerror_s(EINTR));
			break;
		}
		for(int i=0; i<nEvents; i++)
		{
			int fd=evts[i].data.fd;
			if(fd==listenFd)
			{
				struct sockaddr_in4 cliAddr;
				int connfd=connect(listenFd, (struct sockaddr*)&cliAddr, sizeof(cliAddr));
				if(connfd<0)
				{
					printf("connect failed: %s\n", strerror_s(errno));
					break;
				}
				// 将连接挂到子reactor上
				for(int x=0; x<WORKERS; x++)
				{
					if(workers[x].busy==false)
						break;
				}
				if(x<WORKERS  &&  workers[x].busy==false)
				{
					send(pipes[x][0], (char*)&connfd, sizeof(connfd), 0);
				}
				else
				{
					printf("server is busy\n");
					close(connfd);
				}
			}
			else if(fd==sigFd)
			{
				
			}
			else
			{
				printf("other events happend: %d\n", fd);
			}
		}
	}
	
	close(listenFd);
	
	return 0;
}