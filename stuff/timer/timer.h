

// 定时器
// 一个定时器包含：到期时间、回调函数
// 定时器的组织：升序链表、时间轮、时间堆/二叉堆、二叉搜索树set/map
// 定时器的使用方式：
// （1）设置epoll_wait的timeout，timeout时处理超时定时器
// （2）使用timerfd通知epoll_wait，统一事件源（同IO、信号事件）


// vector:随机访问快
// list:插入开销小

#include<functional>
#include<time.h>

typedef std::function<void()> TimerCallback

class Timer{
private:
	time_t due;// TODO:提高精度
	TimeCallback tcb;
	Timer* next;
	
public:

};

