
#include"timerlist.h"

#include<functional>
#include<algorithm>
#include<sstream>

#include"logging.muduo/Logging.h"

TimerList::TimerList()
{}

TimerList::~TimerList()
{}

time_t TimerList::getMinExpired()
{
	if(timers.empty())
		return -1;
	return timers.front()->getDue();
}


void TimerList::handleExpired(time_t now)
{
	for(std::list<Timer*>::iterator it=timers.begin(); it!=timers.end(); )
	{
		Timer* temp=*it;
		if(temp->getDue()<=now)
		{
			temp->run();
			it=timers.erase(it);//序列容器更新迭代器的方法
		}
		else
			break;
	}
}


// 使用binary_function作为基类的原因：
// 提供了函数对象配接器如not1、not2、bind1st、bind2nd所需要的类型定义
// 提供了这些类型定义，才能被称为可配接的函数对象，例如标准自带的ptr_fun
/*
struct CompareByTime: public binary_function<Timer, Timer, bool> // <functonal> 
{
	bool operator()(const Timer& lhs, const Timer& rhs)
	{
		return lhs.due<rhs.due;
	}
}

struct CompareByTimePtr: public binary_function<const Timer*, const Timer*, bool> // <functonal>
{
	bool operator()(const Timer* lhs, const Timer* rhs)
	{
		return lhs->due<rhs->due;
	}
}
*/
/*
void TimerList::handleExpired1(time_t now)
{
	Timer temp(now, std::function());
	std::list<Timer*>::iterator last=std::upper_bound(timers.begin(), timers.end(), &temp, CompareByTimePtr());
	// upper_bound对list来说，比较需要对数次，查找仍然需要线性时间
	
	// last指向第一个时间在now之后的定时器，last之前的定时都超时了
	for(std::list<Timer*>::iterator it=timers.begin(); it!=last; it++)
	{
		Timer *temp=*it;
		temp->run();
	}
	timers.erase(timers.begin(), last);// 线性时间，返回值是last
}

void TimerList::handleExpired2(time_t now)
{
	Timer temp(now, std::function());
	std::list<Timer*>::iterator last=std::upper_bound(timers.begin(), timers.end(), &temp, CompareBytimePtr());
	// upper_bound对list来说，比较需要对数次，查找仍然需要线性时间
	
	// last指向第一个时间在now之后的定时器，last之前的定时都超时了
	for(std::list<Timer*>::iterator it=timers.begin(); it!=last; )
	{
		Timer *temp=*it;
		temp->run();
		
		timers.erase(it++);//erase使得删除元素的迭代器失效，it在删除之前被递增，不受影响
	}
}

void TimerList::handleExpired4(time_t now)
{
	for(std::list<Timer*>::iterator it=timers.begin(); it!=timers.end(); )
	{
		Timer* temp=*it;
		if(temp->getDue()<=now)
		{
			temp->run();
			timers.erase(it++);//关联容器更新迭代器的方法
		}
		else
			break;
	}
}
*/



struct CompareByTimePtr2
{
	bool operator()(const Timer* lhs, const Timer* rhs)
	{
		return lhs->getDue()<rhs->getDue();
	}
};

// 将新定时器按照到期时间插入，到期时间相同的按照插入时间先后排序
void TimerList::addTimer(Timer* timer)
{
	// 插入位置
	//std::list<Timer*>::iterator last=std::upper_bound(timers.begin(), timers.end(), timer, CompareByTimePtr());
	std::list<Timer*>::iterator last=std::upper_bound(timers.begin(), timers.end(), timer, CompareByTimePtr2());
	timers.insert(last, timer);
}

void TimerList::delTimer(Timer* timer)
{
	timers.remove(timer);
}

void TimerList::debugPrint()
{
	std::ostringstream os;
	for(std::list<Timer*>::iterator it=timers.begin(); it!=timers.end(); it++)
	{
		os<<(*it)->getDue()<<", ";
	}
	LOG_DEBUG<<os.str();
}