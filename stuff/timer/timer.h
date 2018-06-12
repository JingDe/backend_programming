

// 定时器
// 一个定时器包含：到期时间、回调函数
// 定时器的组织：可以是链表TimerQueue、时间轮、时间堆。
// 设置一个定时器，设置epoll_wait的timeout，timeout时处理超时定时器


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

