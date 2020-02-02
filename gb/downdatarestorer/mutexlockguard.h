#ifndef DOWN_DATA_RESTORER_MUTEX_LOCK_GUARD_H
#define DOWN_DATA_RESTORER_MUTEX_LOCK_GUARD_H

#include"mutexlock.h"

class MutexLockGuard{
public:
	explicit MutexLockGuard(MutexLock* lock){
		mutex_lock_=lock;
		mutex_lock_.Lock();
	}

	~MutexLockGuard(){
		mutex_lock_.Unlock();
	}

private:
	MutexLock* mutex_lock_;
};

#endif