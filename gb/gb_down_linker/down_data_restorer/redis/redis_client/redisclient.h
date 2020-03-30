#ifndef REDISCLIENT_H
#define REDISCLIENT_H

#include <string>
#include <map>
#include <list>
#include <queue>
#include "dbstream.h"
#include "redisbase.h"
#include "redisconnection.h"
#include "rediscluster.h"
#include "rwmutex.h"
#include "mutexlock.h"
#include "condvar.h"
#include "base_library/log.h"

#include <cassert>
#include <algorithm>

using namespace std;

namespace GBDownLinker {

#define REDIS_DEFALUT_SERVER_PORT 6379
#define REDIS_SLOT_NUM 16384


class RedisConnection;
class RedisMonitor;

enum class RedisClientStatus : uint8_t {
	RedisClientNormal = 0,
	RedisClientRecoverable,
	RedisClientUnrecoverable,
};

enum RedisClientInitResult : uint8_t {
	InitSuccess=0,
	RecoverableFail,
	UnrecoverableFail,
};

struct SentinelStats {
	int sentinelCount;
	int connectedCount;
	int subsribedCount;
};


std::ostream& operator<<(std::ostream& os, enum RedisClientStatus status)
{
	switch (status)
	{
	case RedisClientStatus::RedisClientNormal:
		os<<"Normal";
	case RedisClientStatus::RedisClientRecoverable:
		os<<"RecoverableException";
	case RedisClientStatus::RedisClientUnrecoverable:
		os<<"UnrecoverableException";
	}
	return os;
}

typedef struct RedisServerInfoTag
{
	RedisServerInfoTag()
	{
		serverIp.clear();
		serverPort = 0;
	};
	RedisServerInfoTag(const string& ip, uint32_t port)
	{
		serverIp = ip;
		serverPort = port;
	}
	string serverIp;
	uint32_t serverPort;
}RedisServerInfo;

typedef list<RedisServerInfo> REDIS_SERVER_LIST;
//
typedef struct RedisClusterInfoTag
{
	RedisClusterInfoTag()
	{
		clusterId.clear();
		connectIp.clear();
		connectPort = 0;
		connectionNum = 0;
		connectTimeout = 0;
		readTimeout = 0;
		isMaster = false;
		isAlived = false;
		masterClusterId.clear();
		slotMap.clear(); //key is start slot number,value is stop slot number
		bakClusterList.clear();
		clusterHandler = NULL;
		scanCursor = 0;
	}
	string clusterId;
	string connectIp;
	uint32_t connectPort;
	uint32_t connectionNum;
	uint32_t connectTimeout;
	uint32_t readTimeout;
	bool isMaster;
	bool isAlived;
	string masterClusterId;
	map<uint16_t, uint16_t> slotMap;
	list<string> bakClusterList;
	RedisCluster* clusterHandler;
	int32_t scanCursor;
}RedisClusterInfo;

typedef struct RedisProxyInfoTag
{
	RedisProxyInfoTag()
	{
		proxyId.clear();
		connectIp.clear();
		connectPort = 0;
		connectionNum = 0;
		connectTimeout = 0;
		readTimeout = 0;
		clusterHandler = NULL;
		isAlived = false;
	}
	string proxyId;
	string connectIp;
	uint32_t connectPort;
	uint32_t connectionNum;
	uint32_t connectTimeout;
	uint32_t readTimeout;
	RedisCluster* clusterHandler;
	bool isAlived; // true if has created RedisConnection pool
	bool subscribed; // true if +switch-master channel subscribed
}RedisProxyInfo;


//for redis Optimistic Lock
typedef struct RedisLockInfoTag
{
	RedisLockInfoTag()
	{
		clusterId.clear();
		connection = NULL;
	}
	string clusterId;
	RedisConnection* connection;
}RedisLockInfo;

typedef map<string, RedisClusterInfo> REDIS_CLUSTER_MAP;

typedef map<uint16_t, string> REDIS_SLOT_MAP; // key is slot, value is clusterId


enum RedisCommandType {
	REDIS_COMMAND_UNKNOWN = 0,

	REDIS_COMMAND_AUTH,

	REDIS_COMMAND_READ_TYPE_START,	// read type cmd
	REDIS_COMMAND_GET,
	REDIS_COMMAND_EXISTS,
	REDIS_COMMAND_DBSIZE,
	REDIS_COMMAND_ZCOUNT,
	REDIS_COMMAND_ZCARD,
	REDIS_COMMAND_ZSCORE,
	REDIS_COMMAND_ZRANGEBYSCORE,
	REDIS_COMMAND_SCARD,
	REDIS_COMMAND_SISMEMBER,
	REDIS_COMMAND_SMEMBERS,
	REDIS_COMMAND_KEYS,
	REDIS_COMMAND_READ_TYPE_END,	// read type cmd

	REDIS_COMMAND_WRITE_TYPE_START, // write type cmd
	REDIS_COMMAND_SET,
	REDIS_COMMAND_DEL,
	REDIS_COMMAND_EXPIRE,
	REDIS_COMMAND_ZADD,
	REDIS_COMMAND_ZREM,
	REDIS_COMMAND_ZINCRBY,
	REDIS_COMMAND_ZREMRANGEBYSCORE,
	REDIS_COMMAND_SADD,
	REDIS_COMMAND_SREM,
	REDIS_COMMAND_WRITE_TYPE_END,	// write type cmd

	REDIS_COMMAND_FOR_STAND_ALONE_MODE_START,
	REDIS_COMMAND_WATCH,
	REDIS_COMMAND_UNWATCH,
	REDIS_COMMAND_MULTI,
	REDIS_COMMAND_EXEC,
	REDIS_COMMAND_DISCARD,
	REDIS_COMMAND_FOR_STAND_ALONE_MODE_END,

	REDIS_COMMAND_TO_SENTINEL_NODES_START,	// command to sentinel nodes
	REDIS_COMMAND_SENTINEL_GET_MASTER_ADDR,
	REDIS_COMMAND_INFO_REPLICATION,
	REDIS_COMMAND_SENTINEL_CKQUORUM,
	REDIS_COMMAND_TO_SENTINEL_NODES_END,	// command to sentinel nodes
};



inline bool IsCommandReadType(RedisCommandType type)
{
	return static_cast<int>(type) > static_cast<int>(REDIS_COMMAND_READ_TYPE_START) && static_cast<int>(type) < static_cast<int>(REDIS_COMMAND_READ_TYPE_END);
}

inline bool IsCommandWriteType(RedisCommandType type)
{
	return (int)type > (int)REDIS_COMMAND_WRITE_TYPE_START && (int)type < (int)REDIS_COMMAND_WRITE_TYPE_END;
}

class RedisClient;

struct SwitchMasterThreadArgType
{
	RedisConnection* con;
	RedisClient* client;
};

extern void DefaultCallback(RedisClientStatus);


class RedisClient
{
public:
	enum ScanMode {
		SCAN_LOOP,
		SCAN_NOLOOP,
	};

	RedisClient();
	~RedisClient();

	static std::string ToDesc(RedisClientStatus status)
	{
		switch (status)
		{
		case RedisClientStatus::Normal:
			return "Normal";
		case RedisClientStatus::Exception:
			return "Exception";
		default:
			return "Unknown";
		}
	}

	RedisMode GetRedisMode() {
		return m_redisMode;
	}
	bool init(const string& serverIp, uint32_t serverPort, uint32_t connectionNum, uint32_t connectTimeout = 1200, uint32_t read_timeout_ms = 3000, const string& passwd = "");
	bool init(const REDIS_SERVER_LIST& clusterList, uint32_t connectionNum, uint32_t connectTimeout = 1200, uint32_t read_timeout_ms = 3000, const string& passwd = "");
	bool init(const REDIS_SERVER_LIST& sentinelList, const string& masterName, uint32_t connectionNum, uint32_t connectTimeout = 1200, uint32_t read_timeout_ms = 3000, const string& passwd = "");
	bool init(RedisMode redis_mode, const REDIS_SERVER_LIST& serverList, const string& masterName, uint32_t connectionNum, uint32_t connectTimeout, uint32_t readTimeout, const string& passwd);
	void SetCallback(std::function<void(RedisClientStatus)>());
	bool freeRedisClient();
	template<typename DBSerialize>
	bool getSerial(const string& key, DBSerialize& serial);
	template<typename DBSerialize>
	bool getSerialWithLock(const string& key, DBSerialize& serial, RedisLockInfo& lockInfo);
	bool find(const string& key);
	bool delKeys(const string& delKeys);
	template<typename DBSerialize>
	bool setSerial(const string& key, const DBSerialize& serial);
	bool setSerial(const string& key, const string& serial);
	template<typename DBSerialize>
	bool setSerialWithLock(const string& key, const DBSerialize& serial, RedisLockInfo& lockInfo);
	template<typename DBSerialize>
	bool setSerialWithExpire(const string& key, const DBSerialize& serial, uint32_t expireTime);
	//need unwatch key.
	bool releaseLock(const string& key, RedisLockInfo& lockInfo);
	bool del(const string& key);
	bool setKeyExpireTime(const string& key, uint32_t expireTime);

	bool scanKeys(const string& queryKey, uint32_t count, list<string>& keys, ScanMode scanMode);
	//for query operation.
	bool getKeys(const string& queryKey, list<string>& keys);
	bool getKeysInCluster(const string& queryKey, list<string>& keys);
	//	bool getSerials(const string& key, map<string,RedisSerialize> &serials);

	bool zadd(const string& key, const string& member, int score);
	bool zrem(const string& key, const string& member);
	bool zincby(const string& key, const string& member, int increment);
	int zcount(const string& key, int start, int end);
	int zcard(const string& key);
	int dbsize();
	int zscore(const string& key, const string& member);
	bool zrangebyscore(const string& key, int start, int end, list<string>& members);
	bool zremrangebyscore(const string& key, int start, int end);

	bool sadd(const string& key, const string& member);
	bool srem(const string& key, const string& member);
	int scard(const string& key);
	bool sismember(const string& key, const string& member);
	bool smembers(const string& key, list<string>& members);

	bool isRunAsCluster() { return m_redisMode == CLUSTER_MODE; }

	// for redismonitor
	bool checkAndSaveRedisClusters(REDIS_CLUSTER_MAP& clusterMap);
	void releaseUnusedClusterHandler();
	bool getRedisClustersByCommand(REDIS_CLUSTER_MAP& clusterMap);
	void getRedisClusters(REDIS_CLUSTER_MAP& clusterMap);
	bool isConnected() { return m_connected; }

	// Transaction API
	bool PrepareTransaction(RedisConnection** conn);
	bool WatchKeys(const vector<string>& keys, RedisConnection* con);
	bool WatchKey(const string& key, RedisConnection* con);
	bool Unwatch(RedisConnection* con);
	bool StartTransaction(RedisConnection* con);
	bool DiscardTransaction(RedisConnection* con);
	bool ExecTransaction(RedisConnection* con);
	bool FinishTransaction(RedisConnection** conn);
	// commands supported in a transaction
	bool Set(RedisConnection*, const string&, const string&);
	template<typename DBSerialize>
	bool Set(RedisConnection* con, const string& key, const DBSerialize& serial);
	bool Del(RedisConnection*, const string&);
	bool Sadd(RedisConnection*, const string& key, const string& member);
	bool Srem(RedisConnection*, const string& key, const string& member);

	bool StartSentinelHealthCheckTask();
	bool SubscribeSwitchMaster(RedisCluster* cluster);
	static void* SubscribeSwitchMasterThreadFun(void* arg);
	bool ParseSwithMasterMessage(const RedisReplyInfo& replyInfo, RedisServerInfo& masterAddr);
	bool DoSwitchMaster(const RedisServerInfo& masterAddr);

	// debug test
	void DoTestOfSentinelSlavesCommand();
	bool SentinelGetSlavesInfo(RedisCluster* cluster, vector<RedisServerInfo>& slavesAddr);
	bool ParseSentinelSlavesReply(const CommonReplyInfo& replyInfo, vector<RedisServerInfo>& slavesInfo);

private:
	bool getClusterIdFromRedirectReply(const string& redirectInfo, string& clusterId);
	bool getRedisClusterNodes();
	bool parseClusterInfo(RedisReplyInfo& replyInfo, REDIS_CLUSTER_MAP& clusterMap);
	bool parseOneCluster(const string& infoStr, RedisClusterInfo& clusterInfo);
	void parseSlotStr(string& slotStr, uint16_t& startSlotNum, uint16_t& stopSlotNum);
	bool initRedisCluster();
	bool freeRedisCluster();
	bool initRedisProxy();
	bool freeRedisProxy();

	bool checkIfNeedRedirect(RedisReplyInfo& replyInfo, bool& needRedirect, string& redirectInfo);
	template<typename DBSerialize>
	bool parseGetSerialReply(RedisReplyInfo& replyInfo, DBSerialize& serial, bool& needRedirect, string& redirectInfo);
	bool parseSetSerialReply(RedisReplyInfo& replyInfo, bool& needRedirect, string& redirectInfo);
	//find and del and expire use this reply,for it reply is same.
	bool parseFindReply(RedisReplyInfo& replyInfo, bool& needRedirect, string& redirectInfo);
	bool parseStatusResponseReply(RedisReplyInfo& replyInfo, bool& needRedirect, string& redirectInfo);
	bool parseExecReply(RedisReplyInfo& replyInfo, bool& needRedirect, string& redirectInfo);
	bool parseScanKeysReply(RedisReplyInfo& replyInfo, list<string>& keys, int& retCursor);
	bool parseKeysCommandReply(RedisReplyInfo& replyInfo, list<string>& keys);

	void freeReplyInfo(RedisReplyInfo& replyInfo);
	void freeReplyInfo(CommonReplyInfo& replyInfo);
	void freeCommandList(list<RedisCmdParaInfo>& paraList);

	void fillCommandPara(const char* paraValue, int32_t paraLen, list<RedisCmdParaInfo>& paraList);
	void fillScanCommandPara(int cursor, const string& queryKey, int count, list<RedisCmdParaInfo>& paraList, int32_t& paraLen, ScanMode scanMode);

	//add for get one cluster info
	bool getRedisClusterInfo(string& clusterId, RedisClusterInfo& clusterInfo);
	void updateClusterCursor(const string& clusterId, int newcursor);
	bool getClusterIdBySlot(uint16_t slotNum, string& clusterId);

	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType);
	template<typename DBSerialize>
	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType, DBSerialize* serial);
	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType, int* count);
	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType, list<string>& members);

	// complete declaration
	template<typename DBSerialize>
	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	// do command under 3 modes
	template<typename DBSerialize>
	bool doRedisCommandStandAlone(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	template<typename DBSerialize>
	bool doRedisCommandMaster(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	template<typename DBSerialize>
	bool ParseRedisReplyForStandAloneAndMasterMode(RedisReplyInfo& replyInfo,
		bool& needRedirect,
		string& redirectInfo,
		const string& key,
		RedisCommandType commandType,
		list<string>& members,
		DBSerialize* serial, int* count);
	template<typename DBSerialize>
	bool doRedisCommandCluster(const string& key, int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	//	template<typename DBSerialize>	
	//	bool doRedisCommandWithLock(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, int commandType, RedisLockInfo& lockInfo, bool getSerial = false, DBSerialize* serial = NULL);
	bool doMultiCommand(int32_t commandLen, list<RedisCmdParaInfo>& commandList, RedisConnection** conn);

	// for Transaction API
	bool doTransactionCommandInConnection(int32_t commandLen, list<RedisCmdParaInfo>& commandList, int commandType, RedisConnection* con);
	bool parseStatusResponseReply(RedisReplyInfo& replyInfo);
	bool parseQueuedResponseReply(RedisReplyInfo& replyInfo);
	bool parseExecReply(RedisReplyInfo& replyInfo);

	bool initSentinels();
	bool initMasterSlaves();
	bool SentinelGetMasterAddrByName(RedisCluster* cluster, RedisServerInfo& serverInfo);
	bool ParseSentinelGetMasterReply(const RedisReplyInfo& replyInfo, RedisServerInfo& serverInfo);
	bool MasterGetReplicationSlavesInfo(RedisCluster* cluster, vector<RedisServerInfo>& slaves);
	bool ParseInfoReplicationReply(const RedisReplyInfo& replyInfo, vector<RedisServerInfo>& slaves);
	bool StopSentinelThreads();
	bool freeSentinels();
	bool freeMasterSlaves();
	bool ParseSubsribeSwitchMasterReply(const RedisReplyInfo& replyInfo);
	
	static void* SentinelHealthCheckTask(void* arg);
	bool CheckMasterRole(const RedisServerInfo& masterAddr);
	bool SentinelReinit(SentinelStats& stats);
	
	void SignalToDoMasterCheck();
	static void* CheckMasterNodeThreadFunc(void* arg);
	void DoCheckMasterNode();

	bool StartCheckClusterThread();
	void SignalToDoClusterNodes();
	static void* CheckClusterNodesThreadFunc(void* arg);
	void DoCheckClusterNodes();

	
	bool CreateConnectionPool(RedisProxyInfo&);

	bool AuthPasswd(const string& passwd, RedisCluster* cluster);
	bool ParseAuthReply(const RedisReplyInfo& replyInfo);

private:
	// for all RedisMode
	RedisMode m_redisMode;
	uint32_t m_connectionNum; // data nodes connection pool size
	uint32_t m_connectTimeout;
	uint32_t m_readTimeout;
	string m_passwd;
	bool m_connected;

	// for STAND_ALONE_OR_PROXY_MODE
	RedisProxyInfo m_redisProxy;
	//	RWMutex m_rwProxyMutex;	
	// end for STAND_ALONE_OR_PROXY_MODE

	// for CLUSTER_MODE
	REDIS_SERVER_LIST m_serverList; // redis data nodes
	REDIS_SLOT_MAP m_slotMap;
	RWMutex	m_rwSlotMutex;
	REDIS_CLUSTER_MAP m_clusterMap; // key=clusterId=ip:port
	RWMutex	m_rwClusterMutex;
	map<string, RedisCluster*> m_unusedHandlers;
	//    RedisMonitor* m_redisMonitor;
	pthread_t m_checkClusterNodesThreadId;
	bool m_checkClusterNodesThreadStarted;
	queue<int> m_checkClusterSignalQueue;
	MutexLock m_lockCheckClusterSignalQueue;
	CondVar m_condCheckClusterSignalQueue;
	// end for CLUSTER_MODE

	// for SENTINEL_MODE
	REDIS_SERVER_LIST m_sentinelList; // redis sentinel nodes addr
	string m_masterName;
	
	map<string, RedisProxyInfo> m_sentinelHandlers;
	RWMutex m_rwSentinelHandlers; // guard m_sentinelHandlers
	
	bool m_initMasterAddrGot;
	RedisServerInfo m_initMasterAddr;
	
	map<string, RedisProxyInfo> m_dataNodes; // key: clusterId
	string m_masterClusterId; // current master
	RWMutex m_rwMasterMutex; // guard m_masterClusterId and m_dataNodes

	SwitchMasterThreadArgType m_threadArg;
	vector<pthread_t> m_subscribeThreadIdList;
	RWMutex m_rwSubscribeThreadIdMutex; // guard m_subscribeThreadIdList
	bool m_forceSubscribeThreadExit;

	// for sentinel nodes health check
	pthread_t m_sentinelHealthCheckThreadId;
	bool m_sentinelHealthCheckThreadStarted;
	bool m_forceSentinelHealthCheckThreadExit;

	// for master node health check
	// when master node disconnect, try to get the actual master and re-connect
	pthread_t m_checkMasterNodeThreadId; 
	bool m_checkMasterNodeThreadStarted;
	bool m_forceCheckMasterNodeThreadExit;

	// when doRedisCommand find master disconnected, notify m_checkMasterNodeThreadId
	std::queue<int> m_checkMasterSignalQueue; 
	MutexLock m_lockCheckMasterSignalQueue; // guard m_checkMasterSignalQueue
	CondVar m_condCheckMasterSignalQueue;
	// end for SENTINEL_MODE

	// internal status check and report
	RedisClientStatus m_workStatus;
	std::function<void(RedisClientStatus)> m_callback;
}; // Class RedisClient




template<typename DBSerialize>
bool RedisClient::getSerial(const string& key, DBSerialize& serial)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("get", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	bool success = false;
	success = doRedisCommand(key, paraLen, paraList, RedisCommandType::REDIS_COMMAND_GET, &serial);
	freeCommandList(paraList);
	return success;
}


template<typename DBSerialize>
bool RedisClient::setSerial(const string& key, const DBSerialize& serial)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("set", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	DBOutStream out;
	//serial.save(out);
	save(serial, out);
	fillCommandPara(out.getData(), out.getSize(), paraList);
	paraLen += out.getSize() + 20;
	bool success = false;
	success = doRedisCommand(key, paraLen, paraList, RedisCommandType::REDIS_COMMAND_SET);
	freeCommandList(paraList);
	return success;
}

template<typename DBSerialize>
bool RedisClient::setSerialWithExpire(const string& key, const DBSerialize& serial, uint32_t expireTime)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("setex", 5, paraList);
	paraLen += 17;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	//add expire time
	string expireStr = toStr(expireTime);
	fillCommandPara(expireStr.c_str(), expireStr.length(), paraList);
	paraLen += expireStr.length() + 20;
	DBOutStream out;
	//serial.save(out);
	save(serial, out);
	fillCommandPara(out.getData(), out.getSize(), paraList);
	paraLen += out.getSize() + 20;
	bool success = false;
	success = doRedisCommand(key, paraLen, paraList, RedisCommandType::REDIS_COMMAND_SET);
	freeCommandList(paraList);
	return success;
}

template<typename DBSerialize>
bool RedisClient::doRedisCommand(const string& key,
	int32_t commandLen,
	list < RedisCmdParaInfo >& commandList,
	RedisCommandType commandType,
	DBSerialize* serial)
{
	list<string> members;
	return doRedisCommand(key, commandLen, commandList, commandType, members, serial, NULL);
}

template<typename DBSerialize>
bool RedisClient::doRedisCommand(const string& key,
	int32_t commandLen,
	list < RedisCmdParaInfo >& commandList,
	RedisCommandType commandType,
	list<string>& members,
	DBSerialize* serial, int* count)
{
	if (m_redisMode == CLUSTER_MODE)
	{
		return doRedisCommandCluster(key, commandLen, commandList, commandType, members, serial, count);
	}
	else if (m_redisMode == STAND_ALONE_OR_PROXY_MODE)
	{
		return doRedisCommandStandAlone(key, commandLen, commandList, commandType, members, serial, count);
	}
	else if (m_redisMode == SENTINEL_MODE)
	{
		return doRedisCommandMaster(key, commandLen, commandList, commandType, members, serial, count);
	}
	return false;
}

template<typename DBSerialize>
bool RedisClient::doRedisCommandStandAlone(const string& key,
	int32_t commandLen,
	list < RedisCmdParaInfo >& commandList,
	RedisCommandType commandType,
	list<string>& members,
	DBSerialize* serial, int* count)
{
	RedisReplyInfo replyInfo;
	bool needRedirect;
	string redirectInfo;
	if (m_redisProxy.clusterHandler == NULL)
	{
		LOG_WRITE_ERROR("m_redisProxy.clusterHandler is NULL");
		return false;
	}
	if (!m_redisProxy.clusterHandler->doRedisCommand(commandList, commandLen, replyInfo))
	{
		freeReplyInfo(replyInfo);
		std::stringstream log_msg;
		log_msg << "proxy: " << m_redisProxy.proxyId << " do redis command failed.";
		LOG_WRITE_ERROR(log_msg.str());
		return false;
	}

	bool success = ParseRedisReplyForStandAloneAndMasterMode(replyInfo, needRedirect, redirectInfo, key, commandType, members, serial, count);
	freeReplyInfo(replyInfo);
	return success;
}

template<typename DBSerialize>
bool RedisClient::ParseRedisReplyForStandAloneAndMasterMode(
	RedisReplyInfo& replyInfo,
	bool& needRedirect,
	string& redirectInfo,
	const string& key,
	RedisCommandType commandType,
	list<string>& members,
	DBSerialize* serial, int* count)
{
	switch (commandType)
	{
	case RedisCommandType::REDIS_COMMAND_GET:
		if (!parseGetSerialReply(replyInfo, *serial, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] get string reply failed. reply string:" << replyInfo.resultString << ".";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_SET:
	case RedisCommandType::REDIS_COMMAND_AUTH:
		if (!parseSetSerialReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] get string reply failed. reply string:" << replyInfo.resultString << ".";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_EXISTS:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] get string reply failed. reply string:" << replyInfo.resultString << ".";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		//find
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		//not find
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			std::stringstream log_msg;
			log_msg << "not find key:" << key << " in redis db";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_DEL:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] del string reply failed. reply string:" << replyInfo.resultString << ".";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		//del success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		//del failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			std::stringstream log_msg;
			log_msg << "del key:" << key << " from redis db failed.";
			LOG_WRITE_WARNING(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_EXPIRE:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] set expire reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		//set expire success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		//set expire failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			std::stringstream log_msg;
			log_msg << "set key:" << key << " expire failed.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZADD:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] zset add reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		// success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		// failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			std::stringstream log_msg;
			log_msg << "zset key:" << key << " add done,maybe exists.";
			LOG_WRITE_WARNING(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZREM:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] zset rem reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		// success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		// failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			std::stringstream log_msg;
			log_msg << "set key:" << key << " zrem failed.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}

		break;
	case RedisCommandType::REDIS_COMMAND_ZINCRBY:
		// success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_STRING)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		// failed
		else
		{
			std::stringstream log_msg;
			log_msg << "set key: " << key << "zincrby failed.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;

	case RedisCommandType::REDIS_COMMAND_ZREMRANGEBYSCORE:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] zset zremrangebyscore reply failed. reply string:" << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		// success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue > 0)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		// failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			std::stringstream log_msg;
			log_msg << "set key:" << key << " zremrangebyscore failed.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZCOUNT:
	case RedisCommandType::REDIS_COMMAND_DBSIZE:
	case RedisCommandType::REDIS_COMMAND_ZCARD:
	case RedisCommandType::REDIS_COMMAND_SCARD:
	case RedisCommandType::REDIS_COMMAND_SADD:
	case RedisCommandType::REDIS_COMMAND_SREM:
	case RedisCommandType::REDIS_COMMAND_SISMEMBER:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			std::stringstream log_msg;
			log_msg << "parse key:[" << key << "] zset add reply failed. reply string:" << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}

		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER)
		{
			if (count)
				*count = replyInfo.intValue;
			freeReplyInfo(replyInfo);
			return true;
		}
		else
		{
			std::stringstream log_msg;
			log_msg << "key " << key << " commandType " << commandType << ", return error";
			LOG_WRITE_ERROR(log_msg.str());
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZSCORE:
	{
		if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
		{
			std::stringstream log_msg;
			log_msg << "recv redis wrong reply type:[" << replyInfo.replyType << "]";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		list<ReplyArrayInfo>::iterator iter = replyInfo.arrayList.begin();
		if (iter == replyInfo.arrayList.end())
		{
			std::stringstream log_msg;
			log_msg << "reply not have array info.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		if ((*iter).replyType == RedisReplyType::REDIS_REPLY_NIL)
		{
			std::stringstream log_msg;
			log_msg << "get failed,the key not exist.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		char score_c[64] = { 0 };
		memcpy(score_c, (*iter).arrayValue, (*iter).arrayLen);
		if (count == NULL)
			return false;
		*count = atoi(score_c);
		return true;
	}
	break;
	case RedisCommandType::REDIS_COMMAND_ZRANGEBYSCORE:
	case RedisCommandType::REDIS_COMMAND_SMEMBERS:
	case RedisCommandType::REDIS_COMMAND_KEYS:
		if (parseKeysCommandReply(replyInfo, members) == false)
			return false;
		break;
	default:
		std::stringstream log_msg;
		log_msg << "recv unknown command type: " << commandType;
		LOG_WRITE_ERROR(log_msg.str());
		return false;
	}
	return true;
}


template<typename DBSerialize>
bool RedisClient::doRedisCommandMaster(const string& key,
	int32_t commandLen,
	list < RedisCmdParaInfo >& commandList,
	RedisCommandType commandType,
	list<string>& members,
	DBSerialize* serial, int* count)
{
	RedisReplyInfo replyInfo;
	bool needRedirect;
	string redirectInfo;
	bool disconnected = false;

	RedisProxyInfo& masterNode = m_dataNodes[m_masterClusterId];
	if (masterNode.clusterHandler == NULL || masterNode.isAlived == false)
	{
		std::stringstream log_msg;
		log_msg << "master node " << m_masterClusterId << " not alive";
		LOG_WRITE_ERROR(log_msg.str());

		disconnected = true;
	}
	if (!masterNode.clusterHandler->doRedisCommand(commandList, commandLen, replyInfo))
	{
		freeReplyInfo(replyInfo);
		std::stringstream log_msg;
		log_msg << "master: " << masterNode.proxyId << " do redis command failed.";
		LOG_WRITE_ERROR(log_msg.str());
		
		disconnected = true;		
	}

	if (disconnected == true)
	{
		SignalToDoMasterCheck();
		return false;
	}

	bool success = ParseRedisReplyForStandAloneAndMasterMode(replyInfo, needRedirect, redirectInfo, key, commandType, members, serial, count);
	freeReplyInfo(replyInfo);
	return success;
}

template<typename DBSerialize>
bool RedisClient::doRedisCommandCluster(const string& key,
	int32_t commandLen,
	list < RedisCmdParaInfo >& commandList,
	RedisCommandType commandType,
	list<string>& members,
	DBSerialize* serial, int* count)
{
	assert(m_redisMode == CLUSTER_MODE);
	std::stringstream log_msg;

	uint16_t crcValue = crc16(key.c_str(), key.length());
	crcValue %= REDIS_SLOT_NUM;

	string clusterId;
	if (getClusterIdBySlot(crcValue, clusterId) == false)
	{
		log_msg.str("");
		log_msg << "key:[" << key << "] hit slot:[" << crcValue << "] select cluster failed.";
		LOG_WRITE_ERROR(log_msg.str());
		return false;
	}
	log_msg.str("");
	log_msg << "key:[" << key << "] hit slot:[" << crcValue << "] select cluster[" << clusterId << "].";
	LOG_WRITE_INFO(log_msg.str());

	//add for redirect end endless loop;
	vector<string> redirects;
	RedisReplyInfo replyInfo;
	list<string> bakClusterList;
	list<string>::iterator bakIter;

REDIS_COMMAND:
	RedisClusterInfo clusterInfo;
	if (getRedisClusterInfo(clusterId, clusterInfo) == false)
	{
		log_msg.str("");
		log_msg << "key:[" << key << "] hit slot:[" << crcValue << "], but not find cluster:[" << clusterId << "].";
		LOG_WRITE_ERROR(log_msg.str());
		return false;
	}

	if (!clusterInfo.clusterHandler->doRedisCommand(commandList, commandLen, replyInfo))
	{
		freeReplyInfo(replyInfo);
		log_msg.str("");
		log_msg << "cluster:" << clusterId << " do redis command failed.";
		LOG_WRITE_ERROR(log_msg.str());

		if (IsCommandWriteType(commandType))
		{
			return false;
		}

		//need send to another cluster. check bak cluster.
		if (bakClusterList.empty() == false)
		{
			bakIter++;
			if (bakIter != bakClusterList.end())
			{
				clusterId = (*bakIter);
				goto REDIS_COMMAND;
			}
			else
			{
				log_msg.str("");
				log_msg << "key:[" << key << "] send to all bak cluster failed";
				LOG_WRITE_ERROR(log_msg.str());
				return false;
			}
		}
		else
		{
			bakClusterList = clusterInfo.bakClusterList;
			bakIter = bakClusterList.begin();
			if (bakIter != bakClusterList.end())
			{
				clusterId = (*bakIter);
				goto REDIS_COMMAND;
			}
			else
			{
				log_msg.str("");
				log_msg << "key:[" << key << "] send to all bak cluster failed";
				LOG_WRITE_ERROR(log_msg.str());
				return false;
			}
		}
	}

	bool needRedirect = false;
	string redirectInfo;

	switch (commandType)
	{
	case RedisCommandType::REDIS_COMMAND_GET:
		if (!parseGetSerialReply(replyInfo, *serial, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] get string reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_SET:
	case RedisCommandType::REDIS_COMMAND_AUTH:
		if (!parseSetSerialReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] get string reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_EXISTS:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] get string reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		//find
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		//not find
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			log_msg.str("");
			log_msg << "not find key:" << key << " in redis db";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_DEL:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] del string reply failed. reply string:" << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		//del success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		//del failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			log_msg.str("");
			log_msg << "del key:" << key << " from redis db failed.";
			LOG_WRITE_WARNING(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_EXPIRE:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] set expire reply failed. reply string:" << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		//set expire success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		//set expire failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			log_msg.str("");
			log_msg << "set key:" << key << " expire failed.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZADD:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] zset add reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		// success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		// failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			log_msg.str("");
			log_msg << "zset key:" << key << " add done,maybe exists.";
			LOG_WRITE_INFO(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZREM:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] zset rem reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		// success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		// failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			log_msg.str("");
			log_msg << "set key:" << key << " zrem failed.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}

		break;
	case RedisCommandType::REDIS_COMMAND_ZINCRBY:
		if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "need direct to cluster:[" << redirectInfo << "]";
			LOG_WRITE_INFO(log_msg.str());
		}
		else
		{
			// success
			if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_STRING)
			{
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else
			{
				log_msg.str("");
				log_msg << "set key:" << key << " zincrby failed.";
				LOG_WRITE_ERROR(log_msg.str());
				freeReplyInfo(replyInfo);
				return false;
			}
		}
		break;

	case RedisCommandType::REDIS_COMMAND_ZREMRANGEBYSCORE:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] zset zremrangebyscore reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		// success
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue > 0)
		{
			freeReplyInfo(replyInfo);
			return true;
		}
		// failed
		else if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
		{
			log_msg.str("");
			log_msg << "set key:" << key << " zremrangebyscore failed.";
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZCOUNT:
	case RedisCommandType::REDIS_COMMAND_DBSIZE:
	case RedisCommandType::REDIS_COMMAND_ZCARD:
	case RedisCommandType::REDIS_COMMAND_SCARD:
	case RedisCommandType::REDIS_COMMAND_SADD:
	case RedisCommandType::REDIS_COMMAND_SREM:
	case RedisCommandType::REDIS_COMMAND_SISMEMBER:
		if (!parseFindReply(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "parse key:[" << key << "] get reply failed. reply string: " << replyInfo.resultString;
			LOG_WRITE_ERROR(log_msg.str());
			freeReplyInfo(replyInfo);
			return false;
		}

		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER)
		{
			if (count)
				*count = replyInfo.intValue;
			freeReplyInfo(replyInfo);
			return true;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZSCORE:
		if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "need direct to cluster:[" << redirectInfo << "]";
			LOG_WRITE_WARNING(log_msg.str());
		}
		else
		{
			if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
			{
				log_msg.str("");
				log_msg << "recv redis wrong reply type:[" << replyInfo.replyType << "].";
				LOG_WRITE_ERROR(log_msg.str());
				freeReplyInfo(replyInfo);
				return false;
			}
			list<ReplyArrayInfo>::iterator iter = replyInfo.arrayList.begin();
			if (iter == replyInfo.arrayList.end())
			{
				LOG_WRITE_WARNING("reply not have array info.");
				freeReplyInfo(replyInfo);
				return false;
			}
			if ((*iter).replyType == RedisReplyType::REDIS_REPLY_NIL)
			{
				LOG_WRITE_WARNING("get failed,the key not exist.");
				freeReplyInfo(replyInfo);
				return false;
			}
			char score_c[64] = { 0 };
			memcpy(score_c, (*iter).arrayValue, (*iter).arrayLen);
			if (count)
				*count = atoi(score_c);
			return true;
		}
		break;
	case RedisCommandType::REDIS_COMMAND_ZRANGEBYSCORE:
	case RedisCommandType::REDIS_COMMAND_SMEMBERS:
		if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
		{
			log_msg.str("");
			log_msg << "need direct to cluster:[" << redirectInfo << "].";
			LOG_WRITE_INFO(log_msg.str());
		}
		else
		{
			parseKeysCommandReply(replyInfo, members);
		}
		break;
	default:
		log_msg.str("");
		log_msg << "recv unknown command type:" << commandType;
		LOG_WRITE_ERROR(log_msg.str());
		break;
	}

	freeReplyInfo(replyInfo);
	if (needRedirect)
	{
		SignalToDoClusterNodes();

		log_msg.str("");
		log_msg << "key:[" << key << "] need redirect to cluster:[" << redirectInfo << "].";
		LOG_WRITE_INFO(log_msg.str());

		//check cluster redirect if exist.
		vector<string>::iterator reIter;
		reIter = ::find(redirects.begin(), redirects.end(), redirectInfo);
		if (reIter == redirects.end())
		{
			redirects.push_back(redirectInfo);

			clusterId = redirectInfo;
			bakClusterList.clear();
			goto REDIS_COMMAND;
		}
		else
		{
			log_msg.str("");
			log_msg << "redirect:" << redirectInfo << " is already do redis command,the slot:[" << crcValue << "] may be removed by redis.please check it.";
			LOG_WRITE_ERROR(log_msg.str());
			return false;
		}
	}

	return true;
}


template<typename DBSerialize>
bool RedisClient::parseGetSerialReply(RedisReplyInfo& replyInfo, DBSerialize& serial, bool& needRedirect, string& redirectInfo)
{
	if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
	{
		std::stringstream log_msg;
		log_msg << "need direct to cluster:[" << redirectInfo << "].";
		LOG_WRITE_INFO(log_msg.str());
		return true;
	}
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
	{
		std::stringstream log_msg;
		log_msg << "recv redis wrong reply type:[" << replyInfo.replyType << "].";
		LOG_WRITE_ERROR(log_msg.str());
		return false;
	}
	list<ReplyArrayInfo>::iterator iter = replyInfo.arrayList.begin();
	if (iter == replyInfo.arrayList.end())
	{
		LOG_WRITE_ERROR("reply not have array info.");
		return false;
	}
	if ((*iter).replyType == RedisReplyType::REDIS_REPLY_NIL)
	{
		LOG_WRITE_ERROR("get failed,the key not exist.");
		return false;
	}
	DBInStream in((void*)(*iter).arrayValue, (*iter).arrayLen);
	//serial.load(in);
	load(in, serial);
	if (in.m_loadError)
	{
		LOG_WRITE_ERROR("load data from redis error.");
		return false;
	}
	return true;
}


template<typename DBSerialize>
bool RedisClient::Set(RedisConnection* con, const string& key, const DBSerialize& serial)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("set", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	DBOutStream out;
	//serial.save(out);
	save(serial, out);
	fillCommandPara(out.getData(), out.getSize(), paraList);
	paraLen += out.getSize() + 20;
	bool success = doTransactionCommandInConnection(paraLen, paraList, RedisCommandType::REDIS_COMMAND_SET, con);
	freeCommandList(paraList);
	return success;
}

} // namespace GBDownLinker
#endif

