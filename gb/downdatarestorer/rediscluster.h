#ifndef REDISCLUSTER_H
#define REDISCLUSTER_H

#include <string>
#include <map>
#include <list>
#include <vector>
#include "redisconnection.h"
//#include "owlog.h"
//#include "thread/rwmutex.h"

using namespace std;

//
typedef vector<RedisConnection*> REDIS_CONNECTIONS;

class RedisCluster
{
public:
	RedisCluster(); 
	~RedisCluster();
	bool initConnectPool(const string& clusterIp, uint32_t clusterPort, uint32_t connectionNum, uint32_t keepaliveTime);
	bool freeConnectPool();
	bool doRedisCommand(list<RedisCmdParaInfo> &paraList, int32_t paraLen, RedisReplyInfo& replyInfo, RedisConnection::ReplyParserType parserType=RedisConnection::COMMON_PARSER);
	bool doRedisCommandOneConnection(list<RedisCmdParaInfo> &paraList, int32_t paraLen, RedisReplyInfo& replyInfo, bool release, RedisConnection **conn);
	void releaseConnection(RedisConnection *conn);
	//for if connection not can be used, free it and create a new.
	void freeConnection(RedisConnection *conn);
	bool checkIfCanFree();
private:
	RedisConnection* getAvailableConnection();
	bool checkConnectionAlive(RedisConnection *conn);
private:
//	OWLog	m_logger;
	string	m_clusterIp;
	uint32_t 	m_clusterPort;
	uint32_t	m_connectionNum;
	uint32_t 	m_keepaliveTime;
	REDIS_CONNECTIONS m_connections;
	REDIS_CONNECTIONS m_tmpConnections;
//	RWMutex m_lockAvailableConnection;
};

#endif
