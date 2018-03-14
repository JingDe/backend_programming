#ifndef PORT_POSIX_H_
#define PORT_POSIX_H_


#include<pthread.h>
#include<string>

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
		pthread_mutex_t mu_;

		Mutex(const Mutex&);
		void operator=(const Mutext&);
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

	class Thread : noncopyable {
	public:
		typedef std::function<void (void)> ThreadFunc; // �̺߳�������

		explicit Thread(const ThreadFunc& func, std::string name=std::string());
		~Thread();

		void start();
		void join();

		bool started() const { return started_;  }
		const string& name() const { return name_; }

	private:
		void* startThread(void*);

		pthread_t tid_; // �߳�id
				
		ThreadFunc func_; // �̺߳���
		std::string name_; // �߳���

		bool started_;
		bool joined_;
	};

	class ThreadData {
	public:
		typedef std::function<void(void)> ThreadFunc; // �̺߳�������

		ThreadData(ThreadFunc func);
		~ThreadData();

		void runInThread()
		{

			try {
				func_();
			}
			catch (const std::exception& ex)
			{
				fprintf(stderr, "reason: %s\n", ex.what());
				abort();
			}
			catch (...)
			{
				throw;
			}
		}

		friend class Thread;

	private:
		ThreadFunc func_;
	};
}

#endif