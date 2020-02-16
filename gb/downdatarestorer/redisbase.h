#ifndef REDISBASE_H
#define REDISBASE_H

#include <string>
#include <list>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <bitset>
#include <string.h>
//#include "compile.h"
#include <stdlib.h>
#include <malloc.h>

using namespace std;

//for reply 
struct RedisReplyType
{
	enum{
		REDIS_REPLY_UNKNOWN = 0,
		REDIS_REPLY_NIL,
		REDIS_REPLY_ERROR,
		REDIS_REPLY_STATUS,
		REDIS_REPLY_INTEGER,
		REDIS_REPLY_STRING,
		REDIS_REPLY_ARRAY,
	};
};

typedef struct RedisCmdParaInfoTag
{
	//need release paravalue mem after used
	RedisCmdParaInfoTag()
	{
		paraValue = NULL;
		paraLen = 0;
	}
	char* paraValue;
	int32_t paraLen;
}RedisCmdParaInfo;

typedef struct ReplyArrayInfoTag
{
	//need release mem by user.
	ReplyArrayInfoTag()
	{
		replyType = RedisReplyType::REDIS_REPLY_UNKNOWN;
		arrayValue = NULL;
		arrayLen = 0;
	}
	int replyType;
	char* arrayValue;
	int32_t arrayLen;
}ReplyArrayInfo;

typedef struct RedisReplyInfoTag
{
	RedisReplyInfoTag()
	{
		replyType = RedisReplyType::REDIS_REPLY_UNKNOWN;
		resultString.clear();
		intValue = 0;
		arrayList.clear();
	}
	int replyType;
	string resultString;
	int intValue;
	list<ReplyArrayInfo> arrayList;
}RedisReplyInfo;

enum RedisMode{
	STAND_ALONE_OR_PROXY_MODE,
	CLUSTER_MODE,
	SENTINEL_MODE,
};

bool createRedisCommand(list<RedisCmdParaInfo> &paraList, char **cmdString, int32_t& cmdLen);
bool parseRedisReply(char* replyString, RedisReplyInfo& replyInfo);
bool parseArrayInfo(char **arrayString, ReplyArrayInfo& arrayInfo);

#endif

