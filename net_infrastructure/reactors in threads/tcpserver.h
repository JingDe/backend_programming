//
// 弹性数组 AtomicPointer next_[1];
// vector
// 





#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include<string>

#include<netinet/in.h>

class TcpServer{
public:
	enum SERVER_STATE{
		STATE_OK,
		SOCKET_ERROR,
		BIND_ERROR,
		LISTEN_ERROR,
		ACCEPT_ERROR,
		
	};

	TcpServer(const std::string ip, uint16_t port, int workersNum=1);
	~TcpServer();
	
	void setThreadnum(int num)//设置工作线程数，在run之前调用
	{ 
		if(num<1)
			num=1;
		workersNum=num; 
	}
	
	int listen(int num);
	
	void run();
	
private:
	void sighandler(int signo);

	struct sockaddr_in serverAddr;
	std::string ip;//32位IP地址
	uint16_t port;//16位端口
	
	int listenfd;
	int epfd;
	int sigfd[2];
	
	//与工作线程通信管道,pipe[MaxThreadNum][2]
	//int pipes[1][2];// 弹性数组？？
	//int **pipes;// new int[workersNum][2]
	struct pipe_t{
		int pipe[2];
	};
	std::vector<struct pipe_t> pipes;// 访问pipes[i].pipe[0/1]
	
	//Thread* worker;//TODO: ThreadPool workers;
	std::vector<WorkerThread> workers;
	int workersNum;
	int curWorker;//用于round robin选择工作线程
	// TODO:根据线程的负载
	
	
	TimerQueue timers;
	
};

#endif