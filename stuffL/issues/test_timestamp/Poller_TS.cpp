#include"Poller_TS.h"
#include"EventLoop_TS.h"
#include"Channel_TS.h"

Poller::Poller(EventLoop* loop) :ownerLoop_(loop)
{

}

Poller::~Poller()
{}

void Poller::assertInLoopThread() const
{
	ownerLoop_->assertInLoopThread();
}

bool Poller::hasChannel(Channel* channel) const
{
	assertInLoopThread();
	ChannelMap::const_iterator it = channels_.find(channel->fd());
	return it != channels_.end() && it->second == channel;
}