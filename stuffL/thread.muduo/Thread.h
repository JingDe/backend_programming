#ifndef THREAD_H_
#define THREAD_H_

#include"common/noncopyable.h"
#include"thread.muduo/thread_util.h"

#include<functional>
#include<string>

class Thread : noncopyable {
public:
	typedef std::function<void(void)> ThreadFunc; // 线程函数类型

	explicit Thread(const ThreadFunc& func);
	~Thread();

	void start();
	int join();

	bool started() const { return started_; }
	pid_t tid() const { return tid_; }

private:

	pthread_t pthreadId_; // 线程id
	pid_t tid_; // 内核的线程id

	ThreadFunc func_; // 线程函数

	bool started_;
	bool joined_;
};


class ThreadData {
public:
	typedef Thread::ThreadFunc ThreadFunc; // 线程函数类型

	explicit ThreadData(ThreadFunc func, pid_t* tid) :func_(func), tid_(tid) {}
	~ThreadData() {}

	void runInThread()
	{
		*tid_ = gettid();
		tid_ = NULL;

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
	pid_t* tid_;
};

#endif
