

// （2）使用timerfd通知epoll_wait，统一事件源（同IO、信号事件）

class TimerQueue
{
private:
	int timerfd;
	
public:
	//getMinExpired();
	//handleExpired();
	
	resetTimerfd();//调用timerfd_settime重设定时器的通知时间
	readTimerfd();//
};

int main()
{
	int epfd;
	struct epoll_event events[EPEVENTS];
	int timerfd;
	
	epfd=epoll_create(10);
	if(epfd<0)
	{
		LOG_FATAL<<"epoll_create failed";
		return 0;
	}
	
	timerfd=CreateTimerfd();
	addfd(epfd, timerfd);
	
	while(true)
	{
		// 获得最近的定时器到期时间
		int timeout=5*1000; // 毫秒单位
		int nevt=epoll_wait(epfd, events, EPEVENTS, timeout);
		if(nevt<0)
		{
		
			LOG_FATAL<<"epoll_wait failed";
			break;
		}
		
		for(int i=0; i<nevt; i++)
		{
			uint32_t events=events[i].events;
			int fd=events[i].data.fd;
			
			if(fd==timerfd)
			{
				// 处理超时事件
				
				
				resetTimerfd();
			}
		}
		
	}
	
	close(epfd);
	
	return 0;
}