#ifndef THREAD_H_
#define THREAD_H_

#include"common/noncopyable.h"

#include<functional>
#include<string>

class Thread : noncopyable {
public:
	typedef std::function<void(void)> ThreadFunc; // 线程函数类型

	explicit Thread(const ThreadFunc& func, std::string name = std::string());
	~Thread();

	void start();
	void join();

	bool started() const { return started_; }
	const std::string& name() const { return name_; }

private:
	void* startThread(void*);

	pthread_t tid_; // 线程id

	ThreadFunc func_; // 线程函数
	std::string name_; // 线程名

	bool started_;
	bool joined_;
};

#endif
