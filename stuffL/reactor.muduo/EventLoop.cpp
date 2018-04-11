#include"EventLoop.h"
#include"Channel.h"
#include"Poller.h"
#include"TimerQueue.muduo/TimerId.h"
#include"TimerQueue.muduo/TimerQueue.h"

#include<cassert>

#include<unistd.h>
#include<poll.h>
#include<sys/eventfd.h>

#include"logging.muduo/Logging.h"
#include"thread.muduo/CurrentThread.h"
#include"thread.muduo/thread_util.h"
#include"common/MutexLock.h"

__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000; // poll阻塞时间10秒
static const int kMicroSecondsPerSecond = 1000 * 1000;

inline time_t addTime(time_t t, long seconds)
{
	int64_t delta = static_cast<int64_t>(seconds * kMicroSecondsPerSecond); // int64_t = long long
	time_t result = t + delta;
	return result;
}


/*
int eventfd(unsigned int initval, int flags);
创建一个文件描述符用来事件通知
*/
int createEventfd()
{
	int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		LOG_FATAL << "Failed int eventfd";
		abort();
	}
	return evtfd;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

EventLoop::EventLoop()
	:pid_(tid()), 
	looping_(false),
	quit_(false),
	poller_(Poller::newDefaultPoller(this)),
	currentActiveChannel_(NULL),
	timerQueue_(new TimerQueue(this)),
	wakeupFd_(createEventfd()),
	wakeupChannel_(new Channel(this, wakeupFd_)),
	callingPendingFunctors_(false)
{
	if (t_loopInThisThread)
	{
		LOG_ERROR << "EventLoop already exits in this thread";
	}
	else
		t_loopInThisThread = this;

	// wakeupFd_被注册到EventLoop的poller_中，其他线程调用wakeup往wakeupFd_写
	wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
	assert(!looping_);
	t_loopInThisThread = 0;
}

void EventLoop::loop()
{
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;

	quit_ = false;
	LOG_INFO << "EventLoop " << this << " start looping";

	//poll(NULL, 0, 5 * 1000); // 阻塞5000毫秒
	while (quit_ == false)
	{
		activeChannels_.clear();
		pollReturnTime_=poller_->poll(kPollTimeMs, &activeChannels_);
		if (Logger::logLevel() <= Logger::INFO)
			printActiveChannels();

		for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++)
		{
			currentActiveChannel_ = *it;
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;

		doPendingFunctors(); // IO线程被其他线程唤醒执行函数
	}
	LOG_INFO << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::quit()
{
	quit_ = true;
	if (!isInLoopThread())
		wakeup();
}

void EventLoop::assertInLoopThread()
{
	if (!isInLoopThread())
	{
		LOG_FATAL << "AssertInLoopThread failed: EventLoop "<<this<<" was created in "<<pid_<<" , current thread id = "<< tid();
		return;
	}	
}

void EventLoop::updateChannel(Channel* c)
{
	assert(c->ownerLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(c);
}

void EventLoop::removeChannel(Channel* c)
{
	assert(c->ownerLoop() == this);
	assertInLoopThread();
	poller_->removeChannel(c);
}

void EventLoop::printActiveChannels() const 
{
	for (ChannelList::const_iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++)
	{
		const Channel* ch = *it;
		LOG_INFO << "{" << ch->reventsToString() << "}";
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true; // 设置，以允许wakeup

	{
		MutexLock lock(mutex_);
		functors.swap(pendingFunctors_); // 既缩小临界区，又放置functor调用queueInLoop()死锁
	}

	for (size_t i = 0; i < functors.size(); i++)
	{
		functors[i]();
	}
	callingPendingFunctors_ = false;
}

bool EventLoop::isInLoopThread()
{
	pid_t callerId = tid(); // 为caller计算pid_t
	return callerId == pid_;
}

void EventLoop::runInLoop(const Functor& cb)
{
	if (isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(const Functor& cb)
{
	{
		MutexLock lock(mutex_);
		pendingFunctors_.push_back(cb);
	}
	if (!isInLoopThread() || callingPendingFunctors_) // 当不在IO线程，或者正在执行doPendingFunctors()函数时，需要唤醒IO线程
		wakeup();
}

TimerId EventLoop::runAt(const time_t& time, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, time, 0);
}

TimerId EventLoop::runAfter(long delay, const TimerCallback& cb)
{
	time_t now = time(NULL);
	time_t time(addTime(now, delay));
	return runAt(time, cb);
}

TimerId EventLoop::runEvery(long interval, const TimerCallback& cb)
{
	time_t now = time(NULL);
	time_t time(addTime(now, interval));
	return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = write(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
		LOG_ERROR << "EventLoop::handleRead() writes "<<n<<" bytes instead of 8";
}

void EventLoop::handleRead()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
		LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
}