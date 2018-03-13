#ifndef PORT_WIN32_H_
#define PORT_WIN32_H_

#include<windows.h>

namespace port {
	extern int getThreadID();

	class CondVar;

	class Mutex {
	public:
		Mutex();
		~Mutex();

		void Lock();
		void Unlock();
		void AssertHeld();

	private:
		friend class CondVar;
		// 临界区比mutex高效，但是不是可重入的，并且只能同步同一个进程内部的线程
		void* cs_; // 避免包含windows.h

		Mutex(const Mutex&);
		void operator=(const Mutex&);
	};

	class CondVar {
	public:
		explicit CondVar(Mutex* mu);
		~CondVar();
		void Wait(); 
		void Signal();
		void SignalAll();

	private:
		Mutex * mu_;

		Mutex wait_mtx_;
		long waiting_; // 等待sem1_的个数

		void *sem1_;
		void *sem2_;
	};
}

#endif