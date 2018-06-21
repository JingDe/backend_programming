
//（1）设置epoll_wait的timeout，timeout时处理超时定时器

//#include"logging/Logging.h"
#include"Logging.h"

#include<sys/epoll.h>


const static int EPEVENTS=10;




int main()
{
	int epfd;
	struct epoll_event events[EPEVENTS];
	
	epfd=epoll_create(10);
	if(epfd<0)
	{
		LOG_FATAL<<"epoll_create failed";
		return 0;
	}
	
	
	
	while(true)
	{
		// 获得最近的定时器到期时间
		int timeout=getMinExpired(); // 毫秒单位
		int nevt=epoll_wait(epfd, events, EPEVENTS, timeout);
		if(nevt<0)
		{
			LOG_FATAL<<"epoll_wait failed";
			break;
		}
		
		
		if(nevt==0)
		{
			LOG_TRACE<<"timeout";
			// 处理超时定时器
			handleExpired();
		}
		
	}
	
	close(epfd);
	
	return 0;
}