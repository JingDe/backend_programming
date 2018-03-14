#ifndef PORT_H_
#define PORT_H_

#if defined(WIN32)
#include"port/port_win32.h"
#elif defined(POSIX)
#include"port/port_posix.h"
#endif

namespace port {
	class MutexLock {
	public:
		MutexLock(Mutex& mu) :mu_(mu) {
			mu_.Lock();
		}
		~MutexLock() {
			mu_.Unlock();
		}

	private:
		Mutex & mu_;
	};
}


#endif
