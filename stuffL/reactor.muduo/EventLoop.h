#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include"port/port.h"

#include<sys/types.h>
#include<memory>
#include<vector>

class Channel;
class Poller;
class TimerQueue;
class TimerId;


class EventLoop
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

	void runInLoop(const Functor& cb);
	void queueInLoop(const Functor& cb);

	// ��ʱ������
	TimerId runAt(const time_t& time, const TimerCallback& cb);
	TimerId runAfter(long delay, const TimerCallback& cb);
	TimerId runEvery(long interval, const TimerCallback& cb);

	void wakeup();
	void handleRead();

private:	
	void printActiveChannels() const;
	void doPendingFunctors();
	bool isInLoopThread();


	const pid_t pid_;
	bool looping_;
	bool quit_;	

	std::auto_ptr<Poller> poller_;
	time_t pollReturnTime_;

	typedef std::vector<Channel*> ChannelList;
	ChannelList activeChannels_;
	Channel* currentActiveChannel_;
	

	std::unique_ptr<TimerQueue> timerQueue_; // ��ʱ��
	int wakeupFd_; // �����߳���������IO�̣߳�����loop()��poll����
	std::unique_ptr<Channel> wakeupChannel_;

	port::Mutex mutex_;
	std::vector<Functor> pendingFunctors_;
	bool callingPendingFunctors_;
};

#endif
