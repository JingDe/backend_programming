#ifndef THREAD_H_
#define THREAD_H_

#include"common/noncopyable.h"
#include"thread.muduo/thread_util.h"

#include<functional>
#include<string>

class Thread : noncopyable {
public:
	typedef std::function<void(void)> ThreadFunc; // �̺߳�������

	explicit Thread(const ThreadFunc& func);
	~Thread();

	void start();
	int join();

	bool started() const { return started_; }
	pid_t tid() const { return tid_; }

private:

	pthread_t pthreadId_; // �߳�id
	pid_t tid_; // �ں˵��߳�id

	ThreadFunc func_; // �̺߳���

	bool started_;
	bool joined_;
};


class ThreadData {
public:
	typedef Thread::ThreadFunc ThreadFunc; // �̺߳�������

	explicit ThreadData(ThreadFunc func, pid_t* tid) :func_(func), tid_(tid) {}
	~ThreadData() {}

	void runInThread();

	friend class Thread;

private:
	ThreadFunc func_;
	pid_t* tid_;
};

#endif
