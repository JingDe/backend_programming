#include"port_posix.h"
#include"thread.muduo/thread_util.h"

#include<cstring>
#include<string>

namespace port {

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
		PthreadCall("Init mutex", pthread_mutex_init(&mu_, NULL)); // ����charָ�룬���������飬ʹ���ض��ַ��򳤶Ȳ�����֤��Խ��
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
		// �ͷ���
		PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
	}

	void CondVar::waitForSeconds(int secs)
	{
		struct timespec abstime;
		/*struct timespec{
			time_t tv_sec;
			long tv_nsec;
		};*/
		clock_gettime(CLOCK_REALTIME, &abstime); // ����ض�clock��ʱ��

		const int64_t kNanoSecondsPerSecond = 1e9;
		int64_t nanoseconds = static_cast<int64_t>(secs *kNanoSecondsPerSecond);

		abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
		abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

		PthreadCall("wait for seconds", pthread_cond_timedwait(&cv_, &(mu_->mu_), &abstime)); // �ȴ���һ������ʱ��
	}

	void CondVar::Signal()
	{
		PthreadCall("signal", pthread_cond_signal(&cv_));
	}

	void CondVar::SignalAll()
	{
		PthreadCall("once", pthread_cond_broadcast(&cv_));
	}

}
