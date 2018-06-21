
// 时间轮管理timer
// 使用哈希表，插入timer效率更高


// 共N个槽，每个槽是一个timer链表，链表中的定时器无序
// 相邻槽的间隔时间si
// 同一个槽里的timer的超时时间相差N*si的整数倍
// 当前的槽是cs下标，对应时间time(NULL)
// 新timer的超时时间t，相对此刻的时间则为ti=t-time(NULL)，
// 插入到第 (cs+ti/slot_interval)%N 槽，不能整除时，插入到较小的槽。
// 或者插入到第 [cs+(ti+slot_interval-1)/slot_interval]%N 槽，较大的槽。

// 可以使用多个有着不同间隔si的轮子组成
// 大的si转一个槽等于小的si转一圈


/*
	时间轮的定时器组织方式，便于提供tick()接口，
	不便于提供getExpired(time_t)或者handleExpired(time_t)的接口，仍然需要遍历每个槽的链表
*/

#ifndef TIMER_WHEEL_H_
#define TIMER_WHEEL_H_


class tw_timer{
public:

	friend class TimerWheel;
	
private:

	int rotation;
	int time_slot;
	TimerCallback tcb;
	
	tw_timer* prev;
	tw_timer* next;
}


// 只使用一个轮子的时间轮实现：
class TimerWheel{
public:
	TimerWheel();
	~TimerWheel();
	
	// 根据时间t获得超时的timer，从时间轮中清除，更新cs
	//void getExipred(time_t t, std::vector<Timer*> expired);
	void handleExpired(time_t t);
	
	// 获得最早的超时时间
	time_t getMinExpired();
	
	// 插入一个timer
	void addTimer(Timer* timer);
	
	// 删除一个timer
	void delTimer(Timer* timer);
	
	// 时间轮转动一个槽，更新cs，清除上一个cs里超时的timer
	void tick();

private:
	int cur_slot;// 当前槽下标，绝对时间是当前时间，相对时间为si
	Timer* wheel[N];// 
	
	const static N=6;// N个槽
	const static slot_interval=2;// 相邻槽的时间间隔，秒
	
	int hash(int);// 哈希函数，将相对时间的定时器散列到某个下标的槽
};

#endif

