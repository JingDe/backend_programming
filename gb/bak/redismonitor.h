#ifndef REDISMONITOR_H
#define REDISMONITOR_H

#include <string>
#include <list>
#include "timer.h"
#include "redisclient.h"

using namespace std;

class RedisMonitor: public Timer
{
public:
	RedisMonitor(RedisClient* redis_client);
	~RedisMonitor();
	virtual void run(void);	

private:	
//	OWLog				m_logger;
	RedisClient			*m_redisClient;
};

#endif /*REDISMONITOR_H*/
