#ifndef DOWN_DATA_RESTORER_MUTEX_LOCK_H
#define DOWN_DATA_RESTORER_MUTEX_LOCK_H

#include<pthread.h>

class CondVar;

class MutexLock{
	friend class CondVar;
public:
	MutexLock();
	~MutexLock();
	void Lock();
	void Unlock();

private:
	pthread_mutex_t mutex_;

	MutexLock(const MutexLock&);
	MutexLock& operator=(const MutexLock&);
};

#endif
