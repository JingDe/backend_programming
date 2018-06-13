
#include"tcpserver.h"
#include"util.h"

TcpServer::TcpServer(const std::string ip, uint16_t port, int tnum=1)
	:ip(ip), port(port), listenfd(-1), curWorker(0), threadnum(tnum)
{
	
}

TcpServer::~TcpServer()
{
	if(listenfd!=-1)
		close(listenfd);//
	
}

void TcpServer::sighandler(int signo)
{
	//log("SIGNO %d\n", signo);
	int save_errno=errno;
	int msg=signo;
	//char tmp[10];
	//int n=snprintf(tmp, sizeof(tmp), "%d", msg);
	//write(sigfd[1], tmp, strlen(tmp));
	
	write(sigfd[1], (char*)&msg, 1);
	// 对应读取方式 nread=read(sigfd[0], sigbuf, sizeof(sigbuf)); 
	// switch(sigbuf[i])
	
	errno=save_errno;
}

int TcpServer::listen(int num)
{
	listenfd=socket(AF_INET, SOCK_STREAM, 0);
	if(socket<0)
		return SOCKET_ERROR;
	
	setReuseAddr(listenfd);
	//setNonBlocking(listenfd);	
	
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_port=htons(PORT);
	serverAddr.sin_addr.s_addr=inet_addr(IP);
	// inet_aton(IP, &serverAddr.sin_addr);
	if(bind(listenfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))
		return BIND_ERROR;
	if(listen(listenfd, num))
		return LISTEN_ERROR;
	return STATE_OK;
}

void TcpServer::run()
{
	struct epoll_event evts[10];
	int nEvents;
	
	epfd=epoll_create(10);	
	assert(listenfd!=-1);
	addfd(epfd, listenfd, EPOLLIN);
	
	CHECK(pipe(sigfd), "pipe");
	signal(SIGHUP, sighandler);//挂起终端
	signal(SIGPIPE, sighandler);//往读端关闭的管道或者socket写数据
	signal(SIGURG, sighandler);//带外数据到达
	
	setNonBlocking(sigfd[1]);
	addfd(epfd, sigfd[0], EPOLLIN);
	
	while(true)
	{
		nEvents=epoll_wait(epfd, evts, sizeof(evts), -1);
		if(nEvents<0  &&  errno!=EINTR)//EINTR:系统调用阻塞期间被任意信号打断
		{
			printf("epoll_wait failed: %s\n", strerror(EINTR));
			break;
		}
		else if(nEvents==0)
		{
			printstate();
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
				char temp[10];
				int n=snprintf(temp, sizeof temp, "%d", connfd);
				send(pipes[curWorker][0], temp, n, 0);
				curWorker=(curWorker+1)%threadnum;
			}
			else if(fd==sigfd[0])
			{
				char sigbuf[10]={0};
				int nread=read(sigfd[0], sigbuf, sizeof(sigbuf));
				if(nread>0)
				{
					printf("SIGBUF %s, %d\n", sigbuf, nread);
				}
				for(int i=0; i<nread; i++)
				{
					switch(sigbuf[i])
					{
						case SIGHUP:
							printf("SIGHUP\n");
							break;
						case SIGPIPE:
							printf("SIGPIPE\n");
							break;
						case SIGURG:
							printf("SIGURG\n");
							break;
						default:
							printf("unknown signal\n");
							break;
					}
				}
			}
			else
			{
				printf("other events happend: %d\n", fd);
			}
		}
		
		// 处理其他任务，如定时器事件
		
	}
}