#ifndef EVENTLOOPTHREAD_H_
#define EVENTLOOPTHREAD_H_

#include<functional>

#include"common/noncopyable.h"
#include"thread.muduo/Thread.h"
#include"port/port.h"

class EventLoop;


class EventLoopThread : public noncopyable {
public:
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThread(const ThreadInitCallback& cb=ThreadInitCallback());
	~EventLoopThread();

	EventLoop* startLoop();
	
private:
	void threadFunc();

	port::Mutex mutex_;
	port::CondVar cond_;

	EventLoop * loop_;
	Thread thread_;
	bool exiting_;
	ThreadInitCallback callback_;
};

#endif 
