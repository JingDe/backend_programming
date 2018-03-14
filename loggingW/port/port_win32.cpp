#include"port_win32.h"
#include"../logging.h"

#include<cassert>
#include<system_error>

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
		sem1_(::CreateSemaphore(NULL, 0, 10000, NULL)), // 创建信号量，初始count0，最大count10000，初始nonsignaled
		// 每次wait函数释放等待该信号量的一个线程， count减少1
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

		::WaitForSingleObject(sem1_, INFINITE); // 等待sem1_是signaled(count大于0)
		::ReleaseSemaphore(sem2_, 1, NULL); // 增加sem2_的当前count

		mu_->Lock();
	}

	void CondVar::waitForSeconds(int secs)
	{
		mu_->AssertHeld();

		wait_mtx_.Lock();
		++waiting_;
		wait_mtx_.Unlock();

		mu_->Unlock();

		::WaitForSingleObject(sem1_, secs*1000); // 等待sem1_是signaled(count大于0)，等待毫秒
		::ReleaseSemaphore(sem2_, 1, NULL); // 增加sem2_的当前count

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

	Thread::Thread(const ThreadFunc& func, std::string name):
		func_(func),name_(name)
	{

	}

	Thread::~Thread()
	{
		if (thread_)
			delete thread_;
	}

	void Thread::start()
	{
		assert(started_ == false);
		try {
			thread_ = new std::thread(func_); // 构造后立即开始执行func_函数
		}
		catch (const std::system_error& e)
		{
			delete thread_;
			LOG_FATAL << "start std::thread failed";
		}
		started_ = true;
	}

	void Thread::join()
	{
		thread_->join();
		joined_ = true;
	}
}