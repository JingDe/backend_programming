#ifndef PORT_WIN_H_
#define PORT_WIN_H_

#include<string>
#include<functional>
#include<thread>

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
		void waitForSeconds(int secs);
		void Signal();
		void SignalAll();

	private:
		Mutex *mu_;

		Mutex wait_mtx_; // 同步waiting_的访问
		long waiting_; // 等待sem1_的个数

		void *sem1_;
		void *sem2_; // 作用？？
	};

	class Thread {
	public:
		typedef std::function<void(void)> ThreadFunc; // 线程函数类型

		explicit Thread(const ThreadFunc& func, std::string name = std::string());
		~Thread();

		void start();
		void join();

		bool started() const { return started_; }
		const std::string& name() const { return name_; }

		int tid() {
			return port::getThreadID();
		}

	private:
		std::thread *thread_;

		ThreadFunc func_; // 线程函数
		std::string name_; // 线程名

		bool started_;
		bool joined_;
	};

	class AtomicPointer {
	private:
		void* rep_;

	public:
		AtomicPointer() :rep_(nullptr) {}
		explicit AtomicPointer(void* v);
		void* Acquire_Load() const;
		void* Release_Store(void* v);
		void* NoBarrier_Load() const;
		void NoBarrier_Store(void* v);
	};
}

#endif