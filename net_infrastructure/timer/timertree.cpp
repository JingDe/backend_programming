#include"timertree.h"
#include"logging.muduo/Logging.h"

#include<sstream>

TimerTree::TimerTree()
{

}

TimerTree::~TimerTree()
{

}

time_t TimerTree::getMinExpired()
{
    if(timers.empty())
        return -1;
    return timers.begin()->first;
}

void TimerTree::handleExpired(time_t now)
{
    for(std::set<std::pair<time_t, Timer*> >::iterator it=timers.begin();
        it!=timers.end(); )
    {
        std::pair<time_t, Timer*> tmp=*it;
        if(tmp.first<=now)
        {
            tmp.second->run();
            timers.erase(it++);
        }
        else
            break;
    }
}

/*
struct CompareByTime
{
    bool operator()(const std::pair<time_t, Timer*>& lhs, const std::pair<time_t, Timer*>& rhs)
    {
        return lhs.first<rhs.first;
    }
}
*/

void TimerTree::addTimer(Timer* timer)
{
    time_t tt=timer->getDue();
    std::pair<time_t, Timer*> temp(tt, timer);
    //std::set<std::pair<time_t, Timer*> >::iterator it=std::upper_bound(times.begin(), timers.end(), temp, CompareByTime());
    std::set<std::pair<time_t, Timer*> >::iterator it=timers.upper_bound(temp);
    // it可以是end()
    timers.insert(it, temp);
}

void TimerTree::delTimer(Timer* timer)
{
    std::pair<time_t, Timer*> ele(timer->getDue(), timer);
    timers.erase(ele);
}

void TimerTree::debugPrint()
{
    std::ostringstream oss;
    std::set<std::pair<time_t, Timer*> >::iterator it;
    for(it=timers.begin(); it!=timers.end(); it++)
    {
        oss<<(*it).first<<", ";
    }
    LOG_DEBUG<<oss.str();
}
