#include"port/port_posix.h"

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
		PhtreadCall("Init mutex", pthread_mutex_init(&mutex_, NULL)); // 传递char指针，不拷贝数组，使用特定字符或长度参数保证不越界
		// mutex_=PTHREAD_MUTEX_INITIALIZER;
	}

	Mutex::~Mutex()
	{
		PthreadCall("Destroy mutex", pthread_mutex_destroy(&mutex_));
	}

	void Mutex::Lock()
	{
		PthreadCall("Lock mutex", pthread_mutex_lock(&mutex_));
	}

	void Mutex::Unlock()
	{
		PthreadCall("Unlock mutex", pthread_mutex_unlock(&mutex_));
	}

	void Mutex::AssertHeld()
	{

	}
}