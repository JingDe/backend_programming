#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

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

	// 定时器功能
	TimerId runAfter(long delay, const TimerCallback& cb);
	TimerId runAt(time_t time, const TimerCallback& cb);

private:	
	void printActiveChannels() const;
	void doPendingFunctors();

	bool looping_;
	bool quit_;
	const pid_t pid_;

	std::auto_ptr<Poller> poller_;

	typedef std::vector<Channel*> ChannelList;
	ChannelList activeChannels_;

	Channel* currentActiveChannel_;

	time_t pollReturnTime_;

	std::unique_ptr<TimerQueue> timerQueue_; // 定时器
};

#endif
