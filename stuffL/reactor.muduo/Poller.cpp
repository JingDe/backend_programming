#include"Poller.h"
#include"EventLoop.h"

Poller::Poller(EventLoop* loop) :ownerLoop_(loop)
{

}

Poller::~Poller()
{}

void Poller::assertInLoopThread() const
{
	ownerLoop_->assertInLoopThread();
}