

/*
	适用二叉搜索树，std::set或者std::map。
	将定时器按照到期时间排序
	1）使用multimap，std::multimap<time_t, Timer*>
	2）使用pair<time_t, Timer*>, std::set<pair<time_t, Timer*> >
	使用２）

*/

#ifndef TIMER_TREE_H_
#define TIMER_TREE_H_

#include"timer.h"

#include<set>
#include<utility>

class TimerTree{
public:
	TimerTree();
	~TimerTree();

	time_t getMinExpired();
	void handleExpired(time_t);
	void addTimer(Timer*);
	void delTimer(Timer*);

	void debugPrint();

private:
	std::set<std::pair<time_t, Timer*> > timers;// 按照time_t排序
};

#endif
