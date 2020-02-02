#ifndef DOWN_DATA_RESTORER_EXECUTING_CMD_H
#define DOWN_DATA_RESTORER_EXECUTING_CMD_H

#include"executinginvitecmd.h"

class RedisClient;

class ExecutingInviteCmdMgr{
public:
	ExecutingInviteCmdMgr(RedisClient*);
	
	
private:
	RedisClient* redis_client_;
	
};

#define
