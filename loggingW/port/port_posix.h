#ifndef PORT_POSIX_H_
#define PORT_POSIX_H_


#include<pthread.h>


namespace port {

	extern int getThreadID();
	
	class Mutex {
	public:
		Mutex();
		~Mutex();

		void Lock();
		void Unlock();
		void AssertHeld();

	private:
		pthread_mutex_t mutex_;

		Mutex(const Mutex&);
		void operator=(const Mutext&);
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

		pthread_cond_t cv_;
	};

	class Thread : noncopyable {
	public:
		typedef std::function<void*(void*)> ThreadFunc;
		explicit Thread(const ThreadFunc& func, std::string name=string());
		~Thread();



	private:
		pthread_t tid_;

		
		ThreadFunc func_;
		std::string name_;
	};
}

#endif