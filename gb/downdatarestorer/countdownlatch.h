#ifndef DOWN_DATA_RESTORER_COUNTDOWN_LATCH_H
#define DOWN_DATA_RESTORER_COUNTDOWN_LATCH_H

#include<pthead.h>

class CountDownLatch{
public:
	explict CountDownLatch(int count);
	void Wait();
	void CountDown();
	bool IsValid() { return valid_; }

private:
	pthread_mutex_t mutex_;
	pthread_condition_t condition_;
	int count_;
	bool valid_;
	bool mutex_inited_;
	bool cond_inited_;
};

#endif
