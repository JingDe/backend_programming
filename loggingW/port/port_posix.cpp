#include"port_posix.h"

namespace port {
	
	pid_t gettid()
	{
		return syscall(SYS_tid);
	}

	int getThreadID()
	{
		return static_cast<int>(gettid());
	}

	static void PthreadCall(const char* label, int result) // 不需要修改字符数组，const
	{
		if (result)
		{
			fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
			abort();
		}
	}

	Mutex::Mutex()
	{
		PhtreadCall("Init mutex", pthread_mutex_init(&mu_, NULL)); // 传递char指针，不拷贝数组，使用特定字符或长度参数保证不越界
		// mutex_=PTHREAD_MUTEX_INITIALIZER;
	}

	Mutex::~Mutex()
	{
		PthreadCall("Destroy mutex", pthread_mutex_destroy(&mu_));
	}

	void Mutex::Lock()
	{
		PthreadCall("Lock mutex", pthread_mutex_lock(&mu_));
	}

	void Mutex::Unlock()
	{
		PthreadCall("Unlock mutex", pthread_mutex_unlock(&mu_));
	}

	void Mutex::AssertHeld()
	{

	}

	CondVar::CondVar(Mutex* mu) :mu_(mu)
	{
		PthreadCall("Init cv", pthread_cond_init(&cv_, NULL));
		//cv_=PTHREAD_COND_INITIALIZER;
	}

	CondVar::~CondVar()
	{
		PthreadCall("Destroy cv", pthread_cond_destroy(&cv_));
	}

	void CondVar::Wait() 
	{
		// 释放锁
		PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
	}

	void CondVar::waitForSeconds(int secs)
	{
		const struct timespec abstime;
		/*struct timespec{
			time_t tv_sec;
			long tv_nsec;
		};*/
		clock_gettime(CLOCK_REALTIME, &abstime); // 获得特定clock的时间

		const int64_t kNanoSecondsPerSecond = 1e9;
		int64_t nanoseconds = static_cast<int64_t>(secs *kNanoSecondsPerSecond);

		abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
		abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

		PthreadCall("wait for seconds", pthread_cond_timewait(&cv_, &mu_->mu_, &abstime)); // 等待到一个绝对时间
	}

	void CondVar::Signal()
	{
		PthreadCall("signal", pthread_cond_signal(&cv_));
	}

	void CondVar::SignalAll()
	{
		PthreadCall("once", pthread_cond_broadcast(&cv_));
	}

	Thread::Thread(ThreadFunc func， std::string name) :
		func_(func),
		name_(name)
	{
		
	}

	void Thread::start()
	{
		assert(started_ == false);
		// 创建线程执行func_函数 
		ThreadData td(func_);
		if (pthread_create(&tid_, NULL, std::bind(&Thread::startThread, this), &td))
		{
			delete td;
			LOG_FATAL << "Failed in pthread_create";
		}
		else
		{
			started_ = true;
		}
	}

	void* Thread::startThread(void* arg)
	{
		ThreadData* td = static_cast<ThreadData*>(arg);
		/*ThreadFunc func = td->func_;
		func();*/
		td->runInThread();
		delete td;
		return NULL;
	}

	void Thread::join()
	{
		assert(started_);
		assert(!joined_);
		joined_ = true;
		return pthread_join(tid_, NULL);
	}
}