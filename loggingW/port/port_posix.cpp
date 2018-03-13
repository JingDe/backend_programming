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

	static void PthreadCall(const char* label, int result) // ����Ҫ�޸��ַ����飬const
	{
		if (result)
		{
			fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
			abort();
		}
	}

	Mutex::Mutex()
	{
		PhtreadCall("Init mutex", pthread_mutex_init(&mutex_, NULL)); // ����charָ�룬���������飬ʹ���ض��ַ��򳤶Ȳ�����֤��Խ��
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