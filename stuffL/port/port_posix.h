#ifndef PORT_POSIX_H_
#define PORT_POSIX_H_


#include"common/noncopyable.h"

#include<functional>
#include<string>

#include<pthread.h>


namespace port {

	class CondVar;
	
	class Mutex {
	public:
		Mutex();
		~Mutex();

		void Lock();
		void Unlock();
		void AssertHeld();

		friend CondVar;

	private:
		pthread_mutex_t mu_;

		Mutex(const Mutex&);
		void operator=(const Mutex&);
	};


	class CondVar {
	public:
		explicit CondVar(Mutex* mu);
		~CondVar();

		void Wait();
		void waitForSeconds(int secs);
		void Signal();
		void SignalAll();

	private:
		Mutex * mu_;

		pthread_cond_t cv_;
	};

	
}
#endif

