
#include"timerlist.h"

TimerList::TimerList()
{}

TimerList::~TimerList()
{}

time_t TimerList::getMinExpired()
{
	if(list.empty())
		return -1;
	return timers.front().due;
}

// 使用binary_function作为基类的原因：
// 提供了函数对象配接器如not1、not2、bind1st、bind2nd所需要的类型定义
// 提供了这些类型定义，才能被称为可配接的函数对象，例如标准自带的ptr_fun

struct CompareBytime: public binary_function<Timer, Timer, bool> // <functonal>
{
	bool operator()(const Timer& lhs, const Timer& rhs)
	{
		return lhs.due<rhs.due;
	}
}

struct CompareBytimePtr: public binary_function<const Timer*, const Timer*, bool> // <functonal>
{
	bool operator()(const Timer* lhs, const Timer* rhs)
	{
		return lhs->due<rhs->due;
	}
}

/*
	list是双向迭代器，不是随机访问迭代器
*/
void TimerList::handleExpired(time_t now)
{
	Timer temp(now, std::function());
	std::list<Timer*>::iterator last=std::upper_bound(timers.begin(), timers.end(), &temp, CompareBytimePtr());
	// upper_bound对list来说，比较需要对数次，查找仍然需要线性时间
	
	// last指向第一个时间在now之后的定时器，last之前的定时都超时了
	for(std::list<Timer*>::iterator it=timers.begin(); it!=last; it++)
	{
		Timer *temp=*it;
		temp->tcb();
	}
	timers.erase(timers.begin(), last);// 线性时间
}

