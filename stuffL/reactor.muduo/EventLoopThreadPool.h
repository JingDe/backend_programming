#ifndef EVENTLOOPTHREADPOOL_H_
#define EVENTLOOPTHREADPOOL_H_

#include<vector>
#include<functional>

#include"common/noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public noncopyable {
public:
	typedef std::function<void (EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop);
	~EventLoopThreadPool();
	void setThreadNum(int num) { numThreads_ = num;  }
	void start(const ThreadInitCallback& cb=ThreadInitCallback());
	EventLoop* getNextLoop();

	EventLoop* getLoopForHash(size_t hashCode);

	std::vector<EventLoop*> getAllLoops();

	bool started() const { return started_;  }


private:
	EventLoop * baseLoop_;
	bool started_;
	int numThreads_;
	int next_;
	std::vector<EventLoopThread*> threads_; // 线程池
	std::vector<EventLoop*> loops_; // 线程池内运行的EvenLoop
};

#endif
