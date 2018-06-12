
class TcpServer{
public:
	TcpServer();
	~TcpServer();
	
	void setThreadnum();//设置工作线程数
	void run();
	
private:
	struct sockaddr_in servaddr;
	std::string ip;//32位IP地址
	uint16_t port;//16位端口
	
	int epfd;
	int listenfd;
	int [];//与工作线程通信管道
	
	
};