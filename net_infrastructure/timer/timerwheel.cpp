
#include"timerwheel.h"


TimerWheel::TimerWheel()
	:cur_slot(0)
{
	for(int i=0; i<N; i++)
		wheel[i]=0;
}

TimerWheel::~TimerWheel()
{
	for(int i=0; i<N; i++)
	{
		Timer* pt=wheel[i];
		while(pt)
		{
			Timer* tmp=pt->next;
			delete pt;
			pt=tmp;
		}
	}
}

// hash将相对时间为x的定时器，插入到相应的槽中
int TimerWheel::hash(int x)
{
	return (cur_slot+(x+slot_interval-1)/slot_interval) % N;//稍大一个槽
	// return (cur_slot+(x)/slot_interval-1) % N;//稍小一个槽
}

// FIXME
void TimerWheel::getExipred(time_t t, std::vector<Timer*>& expired)
{
	// t相当大，超过N*si的整数倍
	// 仍然只需要最多遍历一圈
	
	time_t now=time(NULL);
	time_t interval=t-now;
	bool oneloop=true;//少于一圈
	int slots;// 需要遍历多少个slots
	if(interval>N*slot_interval)
	{
		oneloop=false;
		slots=N;//需要遍历一圈，从cs开始
	}
	else
	{
		// 从cs遍历到idx
		slots=(idx-cur_slot+N)%N;
	}
	
	// expired.clear();
	// 获得t时刻的定时器的slot位置
	int idx=hash(t);
	int c=0;
	
	for(int i=cur_slot; c<slots/*i<=(idx+N)%N*/; i=(i+1)%N, c++)
	{
		Timer* p=wheel[i];
		Timer* cur=p;
		Timer* pre=p;
		while(cur)
		{
			if(cur->due>t)
				break;
			expired.push_back(cur);
			if(cur==pre)
			{
				pre=cur->next;
				cur=cur->next;
			}
			else
			{
				pre->next=cur->next;
				pre=cur;
				cur=cur->next;
			}
		}
	}
}


/*
void TimerWheel::addTimer(Timer* timer)
{
	int pos=hash(timer->due-time(NULL)); // 将新timer添加到稍大一个槽中
	// 在slot 2, 4, 6...中，timer1插入到slot2中。timer3插入到slot4中。
	// 转动一个槽（经过2秒），timer1到期，timer3未到期。旧槽的timer到期，新槽的timer不到期
	// timer1延迟？？TODO
	Timer* p=wheel[pos];
	Timer* pre=p;
	if(!p)
	{
		p=timer;
		timer->next=NULL;
		return;
	}
	while(p)
	{
		if(p->due<timer->due)
			pre=p, p=p->next;
	}
	timer->next=p;
	pre->next=timer;
}
*/

// 插入timeout秒后的定时器
tw_timer* addTimer(int timeout)
{
	if(timeout<0)
		return NULL;
		
	int ticks=0;// timeout秒后，即需要走ticks个滴答，ticks个槽
	if(timeout<slot_interval)
		ticks=1;
	else
		ticks=timeout/slot_interval;
	
	int rotation=ticks/N;// ticks个滴答，需要转rotation转
	int ts=(cur_slot+(ticks % N)) % N;// 新定时器插入的槽
	tw_timer* timer=new tw_timer(rotation, ts);	
	if(!wheel[ts])
	{
		wheel[ts]=timer;
	}
	else
	{
		timer->next=wheel[ts];
		wheel[ts]->prev=timer;
		wheel[ts]=timer;
	}
	return timer;
}


/*
	时间轮转一个槽，即时间过去了si秒。
	原来cs槽的最小定时器可能超时。新的cs指向下一个槽。
*/
void TimerWheel::tick()
{
	Timer* tmp=wheel[cur_slot];
	while(tmp)
	{
		if(tmp->rotation>0)
		{
			tmp->rotation--;
			tmp=tmp->next;
		}
		else
		{
			tmp->run();
			if(tmp==wheel[cur_slot])
			{
				wheel[cur_slot]=tmp->next;
				delete tmp;
				if(wheel[cur_slot])
					wheel[cur_slot]->prev=NULL;
				tmp=wheel[cur_slot];
			}
			else
			{
				tmp->prev->next=tmp->next;
				if(tmp->next)
					tmp->next->prev=tmp->prev;
				tw_timer* tmp2=tmp->next;
				delete tmp;
				tmp=tmp2;
			}
		}
	}
	cur_slot==++cur_slot % N;
}

void TimerWheel::handleExpired(time_t now)
{
	time_t t=wheel[cur_slot]->due;// FIXME
	
	int count=(now-t)/si;
	while(count-->0)
		tick();
}
