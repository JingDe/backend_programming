#include"port_win32.h"

#include<cassert>

#include<windows.h>

namespace port {

	int getThreadID()
	{
		return GetCurrentThreadId();
	}

	Mutex::Mutex() :cs_(nullptr) {
		assert(!cs_);
		cs_ = static_cast<void*>(new CRITICAL_SECTION());
		::InitializeCriticalSection(static_cast<CRITICAL_SECTION*>(cs_));
		assert(cs_);
	}

	Mutex::~Mutex()
	{
		assert(cs_);
		::DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(cs_));
		delete static_cast<CRITICAL_SECTION*>(cs_);
		cs_ = nullptr;
		assert(!cs_);
	}
	
	void Mutex::Lock()
	{
		assert(cs_);
		::EnterCriticalSection(static_cast<CRITICAL_SECTION*>(cs_));
	}

	void Mutex::Unlock()
	{
		assert(cs_);
		::LeaveCriticalSection(static_cast<CRITICAL_SECTION*>(cs_));
	}

	void Mutex::AssertHeld()
	{
		assert(cs_);
		assert(1); // ??
	}

	CondVar::CondVar(Mutex* mu) :
		waiting_(0),
		mu_(mu),
		sem1_(::CreateSemaphore(NULL, 0, 10000, NULL)), // �����ź�������ʼcount0�����count10000����ʼnonsignaled
		// ÿ��wait�����ͷŵȴ����ź�����һ���̣߳� count����1
		sem2_(::CreateSemaphore(NULL, 0, 10000, NULL))
	{
		assert(mu_);
	}
	
	CondVar::~CondVar()
	{
		::CloseHandle(sem1_);
		::CloseHandle(sem2_);
	}

	void CondVar::Wait()
	{
		mu_->AssertHeld();

		wait_mtx_.Lock();
		++waiting_;
		wait_mtx_.Unlock();

		mu_->Unlock();

		::WaitForSingleObject(sem1_, INFINITE); // �ȴ�sem1_��signaled(count����0)
		::ReleaseSemaphore(sem2_, 1, NULL); // ����sem2_�ĵ�ǰcount

		mu_->Lock();
	}

	void CondVar::waitForSeconds(int secs)
	{
		mu_->AssertHeld();

		wait_mtx_.Lock();
		++waiting_;
		wait_mtx_.Unlock();

		mu_->Unlock();

		::WaitForSingleObject(sem1_, secs*1000); // �ȴ�sem1_��signaled(count����0)���ȴ�����
		::ReleaseSemaphore(sem2_, 1, NULL); // ����sem2_�ĵ�ǰcount

		mu_->Lock();
	}

	void CondVar::Signal()
	{
		wait_mtx_.Lock();
		if (waiting_ > 0)
		{
			--waiting_;

			::ReleaseSemaphore(sem1_, 1, NULL);
			::WaitForSingleObject(sem2_, INFINITE);
		}
		wait_mtx_.Unlock();
	}

	void CondVar::SignalAll()
	{
		wait_mtx_.Lock();
		for (long i = 0; i < waiting_; i++)
		{
			::ReleaseSemaphore(sem1_, 1, NULL);
			while (waiting_ > 0)
			{
				--waiting_;
				::WaitForSingleObject(sem2_, INFINITE);
			}
		}
		wait_mtx_.Unlock();
	}

	Thread::Thread(const ThreadFunc& func, std::string name):thread_(func),name_(name)
	{

	}

	Thread::~Thread()
	{

	}

	void Thread::start()
	{

	}

	void Thread::stop()
	{

	}
}