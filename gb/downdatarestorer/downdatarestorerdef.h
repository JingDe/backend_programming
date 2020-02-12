#ifndef DOWN_DATA_RESTORER_DEF_H
#define DOWN_DATA_RESTORER_DEF_H

#include<list>

using std::list;

class ExecutingInviteCmd;

typedef list<ExecutingInviteCmd> ExecutingInviteCmdList;


enum RedisMode{
	STAND_ALONE=0,
	CLUSTER,
	SENTINEL,
};


#endif
