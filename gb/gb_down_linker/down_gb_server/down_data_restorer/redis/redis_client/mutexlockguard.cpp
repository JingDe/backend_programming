#include"mutexlockguard.h"

namespace GBGateway {

MutexLockGuard::MutexLockGuard(MutexLock* lock){
	mutex_lock_=lock;
	mutex_lock_->Lock();
}

MutexLockGuard::~MutexLockGuard(){
	mutex_lock_->Unlock();
}


} // namespace GBGateway

