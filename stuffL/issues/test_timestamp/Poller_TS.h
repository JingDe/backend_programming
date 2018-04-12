#ifndef POLLER_H_
#define POLLER_H_

#include<map>
#include<vector>

class EventLoop;
class Channel;
class Timestamp;

class Poller {
public:
	typedef std::vector<Channel*> ChannelList;

	static Poller* newDefaultPoller(EventLoop*);

	Poller(EventLoop* loop);
	virtual ~Poller();

	virtual void updateChannel(Channel*) = 0;
	virtual void removeChannel(Channel*) = 0;
	virtual bool hasChannel(Channel* channel) const;
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

	void assertInLoopThread() const;

private:
	EventLoop * ownerLoop_;

protected:
	typedef std::map<int, Channel*> ChannelMap;
	ChannelMap channels_;
};

#endif