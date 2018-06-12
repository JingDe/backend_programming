
// 时间轮管理timer
// 使用哈希表，插入timer效率更高


// 共N个槽，每个槽是一个timer链表
// 相邻槽的间隔时间si
// 同一个槽里的timer的超时时间相差N*si的整数倍
// 当前的槽是cs，新timer的超时时间ti，插入到第 (si+ti/si)%N 槽。[si+(ti+si-1)/si]%N

// 当ti不是si的整数倍，ti的timer被放置在稍小的slot中。
// 所以，检查

// 多个有着不同间隔si的轮子组成
// 大的si转一个槽等于小的si转一圈


// 只使用一个轮子的时间轮实现
class TimerWheel{
public:
	TimerWheel();
	~TimerWheel();
	
	// 根据时间t获得超时的timer，从时间轮中清除，更新cs
	void getExipred(time_t t, std::vector<Timer*> expired);
	
	// 获得最早的超时时间
	time_t getFirst();
	
	// 插入一个timer
	void insertTimer(Timer* timer);
	
	// 删除一个timer
	void delTimer(Timer* timer);
	
	// 时间轮转动一个槽，更新cs，清除上一个cs里超时的timer
	void tick();

private:
	int cs;// 当前槽，其中的timer最早超时时间是si（和相差N*si的整数倍的timer）
	Timer* wheel[N];// 
	const static N=6;// N个槽
	const static si=2;// 相邻槽的时间间隔
	
	int hash(int);
};

TimerWheel::TimerWheel()
	:cs(0)
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

void TimerWheel::getExipred(time_t t, std::vector<Timer*>& expired)
{
	// t相当大，超过N*si的整数倍
	// 仍然只需要最多遍历一圈
	
	time_t now=time(NULL);
	time_t interval=t-now;
	bool oneloop=true;//少于一圈
	int slots;// 需要遍历多少个slots
	if(interval>N*si)
	{
		oneloop=false;
		slots=N;//需要遍历一圈，从cs开始
	}
	else
	{
		// 从cs遍历到idx
		slots=(idx-cs+N)%N;
	}
	
	// expired.clear();
	// 获得t时刻的定时器的slot位置
	int idx=hash(t);
	int c=0;
	
	for(int i=cs; c<slots/*i<=(idx+N)%N*/; i=(i+1)%N, c++)
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

int TimerWheel::hash(int x)
{
	return (cs+(x+si-1)/si-1) % N;//稍大一个槽
	// return (cs+(x)/si-1) % N;//稍小一个槽
}

void TimerWheel::insertTimer(Timer* timer)
{
	int pos=hash(timer->due); // 将新timer添加到稍大一个槽中
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

void TimerWheel::tick()
{
	
}