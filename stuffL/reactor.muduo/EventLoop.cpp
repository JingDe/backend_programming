#include"EventLoop.h"
#include"Channel.h"
#include"Poller.h"
#include"TimerQueue.muduo/TimerId.h"
#include"TimerQueue.muduo/TimerQueue.h"
#include"TimerQueue.muduo/Timestamp.h"

#include<cassert>
#include<algorithm>

#include<unistd.h>
#include<poll.h>
#include<sys/eventfd.h>
#include<signal.h>

#include"logging.muduo/Logging.h"
#include"thread.muduo/CurrentThread.h"
#include"thread.muduo/thread_util.h"
#include"common/MutexLock.h"

__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000; // poll����ʱ��10��


class IgnorePipe {
public:
	IgnorePipe()
	{
		signal(SIGPIPE, SIG_IGN);
	}
};

IgnorePipe initObj;

/*
int eventfd(unsigned int initval, int flags);
����һ���ļ������������¼�֪ͨ
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
    eventHandling_(false),
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

	// wakeupFd_��ע�ᵽEventLoop��poller_�У������̵߳���wakeup��wakeupFd_д
	wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
	LOG_DEBUG << "EventLoop " << this << " of thread " << pid_ << " destructs in thread " << tid();
	assert(!looping_);
	t_loopInThisThread = 0;
	wakeupChannel_->disableAll();
	wakeupChannel_->remove();
	close(wakeupFd_);
}

void EventLoop::loop()
{
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;

	quit_ = false;
	LOG_INFO << "EventLoop " << this << " start looping";

	//poll(NULL, 0, 5 * 1000); // ����5000����
	while (quit_ == false)
	{
		activeChannels_.clear();
		pollReturnTime_=poller_->poll(kPollTimeMs, &activeChannels_);
		if (Logger::logLevel() <= Logger::INFO)
			printActiveChannels();

		eventHandling_ = true;
		for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++)
		{
			currentActiveChannel_ = *it;
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors(); // IO�̱߳������̻߳���ִ�к���
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
	if (eventHandling_)
	{
		assert(currentActiveChannel_ == c || std::find(activeChannels_.begin(), activeChannels_.end(), c) == activeChannels_.end());
	}
	poller_->removeChannel(c);
}

bool EventLoop::hasChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::printActiveChannels() const 
{
	LOG_DEBUG << "timerQueue.timerfd = " << timerQueue_->timerfd();
	for (ChannelList::const_iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++)
	{
		const Channel* ch = *it;
		LOG_INFO << "{" << ch->reventsToString() << "}";
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true; // ���ã�������wakeup

	{
		MutexLock lock(mutex_);
		functors.swap(pendingFunctors_); // ����С�ٽ������ַ���functor����queueInLoop()����
	}

	for (size_t i = 0; i < functors.size(); i++)
	{
		functors[i]();
	}
	callingPendingFunctors_ = false;
}

bool EventLoop::isInLoopThread()
{
	pid_t callerId = tid(); // Ϊcaller����pid_t
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
	if (!isInLoopThread() || callingPendingFunctors_) // ������IO�̣߳���������ִ��doPendingFunctors()����ʱ����Ҫ����IO�߳�
		wakeup();
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
	Timestamp when(addTime(Timestamp::now(), delay));	
	return runAt(when, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
	Timestamp when(addTime(Timestamp::now(), interval));	
	return timerQueue_->addTimer(cb, when, interval);
}

void EventLoop::cancel(TimerId timerId)
{
	return timerQueue_->cancel(timerId);
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