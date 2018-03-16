#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_

#include"port/port.h"

class MutexLock {
public:
	MutexLock(port::Mutex& mu) :mu_(mu) {
		mu_.Lock();
	}
	~MutexLock() {
		mu_.Unlock();
	}

private:
	port::Mutex & mu_; // Mutex* mu_;
};

#endif