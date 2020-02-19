#include "rediscluster.h"
#include"util.h"
#include"glog/logging.h"

#define CONNECTION_RELEASE_TIME 10

RedisCluster::RedisCluster()//:m_logger("cdn.common.rediscluster")
{
	m_clusterIp.clear();
	m_clusterPort = 0;
	m_connectionNum = 0;
	m_keepaliveTime = 0;
	m_connections.clear();
	m_tmpConnections.clear();
}

RedisCluster::~RedisCluster()
{
	m_clusterIp.clear();
	m_clusterPort = 0;
	m_connectionNum = 0;
	m_keepaliveTime = 0;
	m_tmpConnections.clear();
}

bool RedisCluster::initConnectPool(const string & clusterIp,uint32_t clusterPort,uint32_t connectionNum,uint32_t keepaliveTime)
{
	//create redis connection.
	WriteGuard guard(m_lockAvailableConnection);
	for (unsigned int i = 0; i < connectionNum; i++)
	{
		RedisConnection *connection = new RedisConnection(clusterIp, clusterPort, keepaliveTime);
		if(!connection->connect())
		{
			LOG(ERROR)<<"connect to clusterIp:"<<clusterIp<<" clusterPort:"<<clusterPort<<" failed.";
			delete connection;
			return false;
		}
		connection->m_available = true;
		connection->m_connectTime = 0;
		m_connections.push_back(connection);
	}
	m_clusterIp = clusterIp;
	m_clusterPort = clusterPort;
	m_connectionNum = connectionNum;
	m_keepaliveTime = keepaliveTime;
	LOG(INFO)<<"initConnectPool ok, pool size is "<<m_connections.size();
	return true;
}

bool RedisCluster::freeConnectPool()
{
	WriteGuard guard(m_lockAvailableConnection);
	REDIS_CONNECTIONS::iterator iter;
	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		RedisConnection *connection = (*iter);
		if (connection != NULL)
		{
			connection->close();
			delete connection;
			connection = NULL;
		}
	}
	m_connections.clear();
	for (iter = m_tmpConnections.begin(); iter != m_tmpConnections.end(); iter++)
	{
		RedisConnection *connection = (*iter);
		if (connection != NULL)
		{
			connection->close();
			delete connection;
			connection = NULL;
		}
	}
	m_tmpConnections.clear();
	return true;
}

bool RedisCluster::checkIfCanFree()
{
	WriteGuard guard(m_lockAvailableConnection);
	bool findFree = true;
	REDIS_CONNECTIONS::iterator iter;
	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		RedisConnection *conn = *iter;
		if (!conn->m_available)
		{
			findFree = false;
			break;
		}
	}
	return findFree;
}

bool RedisCluster::doRedisCommand(list < RedisCmdParaInfo > & paraList,int32_t paraLen,RedisReplyInfo & replyInfo, RedisConnection::ReplyParserType parserType)
{
	bool success = false;
	RedisConnection* connection = getAvailableConnection();
	if (connection != NULL)
	{
		success = connection->doRedisCommand(paraList, paraLen, replyInfo, parserType);
		releaseConnection(connection);
	}
	else
	{
//		m_logger.warn("doRedisCommand, cluster:[%s:%d] not find available connection, use once connection.",m_clusterIp.c_str(), m_clusterPort);
		connection = new RedisConnection(m_clusterIp, m_clusterPort, 30);
		if(connection==NULL)
			return false;
		if(!connection->connect())
		{
//			m_logger.warn("connect to clusterIp:%s clusterPort:%d failed.", m_clusterIp.c_str(), m_clusterPort);
			delete connection;
			return false;
		}
		success = connection->doRedisCommand(paraList, paraLen, replyInfo, parserType);
		connection->close();
		delete connection;
		connection = NULL;
	}
	return success;
}

bool RedisCluster::doRedisCommandOneConnection(list < RedisCmdParaInfo > & paraList,int32_t paraLen,RedisReplyInfo & replyInfo,bool release,RedisConnection ** conn)
{
	bool success = false;
	if (*conn == NULL)
	{
		*conn = getAvailableConnection();
	}
	if (*conn != NULL)
	{
		success = (*conn)->doRedisCommand(paraList, paraLen, replyInfo);
	}
	else
	{
		*conn = new RedisConnection(m_clusterIp, m_clusterPort, 30);
		if(!(*conn)->connect())
		{
			return false;
		}
		(*conn)->m_connectTime = getCurrTimeSec();
		success = (*conn)->doRedisCommand(paraList, paraLen, replyInfo);
		WriteGuard guard(m_lockAvailableConnection);
		m_tmpConnections.push_back(*conn);
	}
	if (release)
		releaseConnection(*conn);
	return success;
}

RedisConnection* RedisCluster::getAvailableConnection()
{
	WriteGuard guard(m_lockAvailableConnection);
	uint32_t currentTime = getCurrTimeSec();
	REDIS_CONNECTIONS::iterator iter;
	for (iter = m_tmpConnections.begin(); iter != m_tmpConnections.end(); )
	{
		RedisConnection *conn = *iter;
		if (currentTime >= (conn->m_connectTime + CONNECTION_RELEASE_TIME))
		{
			//need close it for clean watch key.
			conn->close();
			delete conn;
			iter = m_tmpConnections.erase(iter);
		}
		else
		{
			iter++;
		}
	}
//	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
//	{
//		RedisConnection *conn = *iter;
//		if (conn->m_available)
//		{
//			conn->m_available = false;
//			conn->m_connectTime = currentTime;
//			return conn;
//		}
//		else
//		{
//			if (currentTime >= (conn->m_connectTime + CONNECTION_RELEASE_TIME))
//			{
//				//need close it for clean watch key.
//				conn->close();
//				delete conn;
//				conn = NULL;
//				//recreate redis connection.
//				conn = new RedisConnection(m_clusterIp, m_clusterPort, m_keepaliveTime);
//				if(!conn->connect())
//				{
////					m_logger.warn("connect to clusterIp:%s clusterPort:%d failed.", m_clusterIp.c_str(), m_clusterPort);
//					return NULL;
//				}
//				conn->m_available = false;
//				conn->m_connectTime = currentTime;
//				*iter = conn;
//				return conn;
//			}
//		}
//	}

	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		RedisConnection *conn = *iter;
		if (conn->m_available)
		{
			conn->m_available = false;
			conn->m_connectTime = currentTime;
			return conn;
		}
	}
	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		RedisConnection *conn = *iter;
		if (conn->CanRelease()  &&  currentTime >= (conn->m_connectTime + CONNECTION_RELEASE_TIME))
		{
			//need close it for clean watch key.
			conn->close();
			delete conn;
			conn = NULL;
			//recreate redis connection.
			conn = new RedisConnection(m_clusterIp, m_clusterPort, m_keepaliveTime);
			if(!conn->connect())
			{
				LOG(ERROR)<<"connect to clusterIp:"<<m_clusterIp<<" clusterPort:"<<m_clusterPort<<" failed.";
				return NULL;
			}
			conn->m_available = false;
			conn->m_connectTime = currentTime;
			*iter = conn;
			return conn;
		}
		
	}
	return NULL;
}

void RedisCluster::releaseConnection(RedisConnection * conn)
{
	WriteGuard guard(m_lockAvailableConnection);
	bool find = false;
	REDIS_CONNECTIONS::iterator iter;
	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		if ((*iter) == conn)
		{
			(*iter)->m_available = true;
			find = true;
			break;
		}
	}
	if (!find)
	{
		bool findTmp = false;
		for (iter = m_tmpConnections.begin(); iter != m_tmpConnections.end(); iter++)
		{
			if ((*iter) == conn)
			{
				conn->close();
				delete conn;
				conn = NULL;
				m_tmpConnections.erase(iter);
				findTmp = true;
				break;
			}
		}
		if (!findTmp)
		{
//			m_logger.warn("not find connection:%p, may be release already.", conn);
		}
	}
}

void RedisCluster::freeConnection(RedisConnection * conn)
{
	WriteGuard guard(m_lockAvailableConnection);
	uint32_t currentTime = getCurrTimeSec();
	REDIS_CONNECTIONS::iterator iter;
	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		//check connection
		if ((*iter) == conn)
		{
			conn->close();
			delete conn;
			conn = NULL;
			//recreate redis connection.
			conn = new RedisConnection(m_clusterIp, m_clusterPort, m_keepaliveTime);
			if(!conn->connect())
			{
//				m_logger.warn("connect to clusterIp:%s clusterPort:%d failed.", m_clusterIp.c_str(), m_clusterPort);
				return;
			}
			conn->m_available = false;
			conn->m_connectTime = currentTime;
			*iter = conn;
			return;
		}
	}
}

bool RedisCluster::checkConnectionAlive(RedisConnection * conn)
{
	WriteGuard guard(m_lockAvailableConnection);
	bool find = false;
	REDIS_CONNECTIONS::iterator iter;
	for (iter = m_connections.begin(); iter != m_connections.end(); iter++)
	{
		if ((*iter) == conn)
		{
			if ((*iter)->m_available)
			{
//				m_logger.warn("the connection:%p already release by other.", conn);
				(*iter)->m_available = false;
			}
			find = true;
			break;
		}
	}
	if (!find)
	{
		bool findTmp = false;
		for (iter = m_tmpConnections.begin(); iter != m_tmpConnections.end(); iter++)
		{
			if ((*iter) == conn)
			{
				findTmp = true;
				break;
			}
		}
	}
	return find;
}

