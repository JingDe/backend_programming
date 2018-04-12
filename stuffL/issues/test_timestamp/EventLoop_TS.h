#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include"port/port.h"
#include"common/noncopyable.h"

#include<memory>
#include<vector>

#include<sys/types.h>

class Channel;
class Poller;
class TimerQueue;
class TimerId;
#include"TimerQueue.muduo/Timestamp.h"

class EventLoop : noncopyable
{
public:
	typedef std::function<void()> Functor;
	typedef std::function<void()> TimerCallback;

	static EventLoop* getEventLoopOfCurrentThread();
	EventLoop();
	~EventLoop();

	void loop();
	void quit();

	void assertInLoopThread();

	void updateChannel(Channel* c);
	void removeChannel(Channel* c);
	bool hasChannel(Channel* channel);

	void runInLoop(const Functor& cb);
	void queueInLoop(const Functor& cb);

	// ��ʱ������
	TimerId runAt(const Timestamp& time, const TimerCallback& cb);
	TimerId runAfter(double delay, const TimerCallback& cb);
	TimerId runEvery(double interval, const TimerCallback& cb);
	void cancel(TimerId timerId);

	void wakeup();
	void handleRead();

private:	
	void printActiveChannels() const;
	void doPendingFunctors();
	bool isInLoopThread();


	const pid_t pid_;
	bool looping_;
	bool quit_;	
	bool eventHandling_;

	std::auto_ptr<Poller> poller_;
	Timestamp pollReturnTime_;

	typedef std::vector<Channel*> ChannelList;
	ChannelList activeChannels_;
	Channel* currentActiveChannel_;
	

	std::unique_ptr<TimerQueue> timerQueue_; // ��ʱ��
	int wakeupFd_; // �����߳���������IO�̣߳�����loop()��poll����
	std::unique_ptr<Channel> wakeupChannel_;

	mutable port::Mutex mutex_;
	std::vector<Functor> pendingFunctors_;
	bool callingPendingFunctors_;
};

#endif
