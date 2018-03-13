#ifndef PORT_WIN32_H_
#define PORT_WIN32_H_

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
		// �ٽ�����mutex��Ч�����ǲ��ǿ�����ģ�����ֻ��ͬ��ͬһ�������ڲ����߳�
		void* cs_; // �������windows.h

		Mutex(const Mutex&);
		void operator=(const Mutex&);
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

		Mutex wait_mtx_;
		long waiting_; // �ȴ�sem1_�ĸ���

		void *sem1_;
		void *sem2_;
	};
}

#endif