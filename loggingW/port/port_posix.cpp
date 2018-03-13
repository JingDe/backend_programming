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
		// �ͷ���
		PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mutex_));
	}

	void CondVar::Signal()
	{
		PthreadCall("signal", pthread_cond_signal(&cv_));
	}

	void CondVar::SignalAll()
	{
		PthreadCall("once", pthread_cond_broadcast(&cv_));
	}

	Thread::Thread(std::string name, ThreadFunc func) :
		name_(name),
		func_(func)
	{
		PthreadCall("create thread", pthread_create(&tid_, NULL, func_, arg));
	}
}