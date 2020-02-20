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

#define REDIS_DEFALUT_SERVER_PORT 6379
#define REDIS_SLOT_NUM 16384

using namespace std;

class RedisConnection;
class RedisMonitor;

typedef struct RedisServerInfoTag
{
	RedisServerInfoTag()
	{
		serverIp.clear();
		serverPort = 0;
	};
    RedisServerInfoTag(const string& ip, uint32_t port)
    {
        serverIp=ip;
        serverPort=port;
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
		keepaliveTime = 0;
		isMaster = false;
		isAlived = false;
		masterClusterId.clear();
		slotMap.clear(); //key is start slot number,value is stop slot number
		bakClusterList.clear();
		clusterHandler = NULL;
		scanCursor=0;
	}
	string clusterId;
	string connectIp;
	uint32_t connectPort;
	uint32_t connectionNum;
	uint32_t keepaliveTime;
	bool isMaster;
	bool isAlived;
	string masterClusterId;
	map<uint16_t,uint16_t> slotMap;
	list<string> bakClusterList;
	RedisCluster *clusterHandler;
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
		keepaliveTime = 0;
		clusterHandler = NULL;
		isAlived=false;
	}
	string proxyId;
	string connectIp;
	uint32_t connectPort;
	uint32_t connectionNum;
	uint32_t keepaliveTime;
	RedisCluster *clusterHandler;
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
	RedisConnection *connection;
}RedisLockInfo;

typedef map<string, RedisClusterInfo> REDIS_CLUSTER_MAP;

typedef map<uint16_t, string> REDIS_SLOT_MAP; // key is slot, value is clusterId
//
//struct RedisCommandType
//{
//	enum{
//		REDIS_COMMAND_UNKNOWN = 0,
//		
//		REDIS_COMMAND_READ_TYPE_START,	// read type cmd
//		REDIS_COMMAND_GET,
//		REDIS_COMMAND_EXISTS,
//		REDIS_COMMAND_DBSIZE,
//		REDIS_COMMAND_ZCOUNT,
//		REDIS_COMMAND_ZCARD,
//		REDIS_COMMAND_ZSCORE,
//		REDIS_COMMAND_ZRANGEBYSCORE,
//		REDIS_COMMAND_SCARD,
//		REDIS_COMMAND_SISMEMBER,
//		REDIS_COMMAND_SMEMBERS,
//		REDIS_COMMAND_READ_TYPE_END,	// read type cmd
//
//		REDIS_COMMAND_WRITE_TYPE_START, // write type cmd
//		REDIS_COMMAND_SET,
//		REDIS_COMMAND_DEL,
//		REDIS_COMMAND_EXPIRE,
//		REDIS_COMMAND_ZADD,
//		REDIS_COMMAND_ZREM,
//		REDIS_COMMAND_ZINCRBY,
//		REDIS_COMMAND_ZREMRANGEBYSCORE,
//		REDIS_COMMAND_SADD,
//		REDIS_COMMAND_SREM,
//		REDIS_COMMAND_WRITE_TYPE_END,	// write type cmd
//		
//		REDIS_COMMAND_FOR_STAND_ALONE_MODE_START,
//		REDIS_COMMAND_WATCH,
//		REDIS_COMMAND_UNWATCH,
//		REDIS_COMMAND_MULTI,
//		REDIS_COMMAND_EXEC,
//		REDIS_COMMAND_DISCARD,		
//		REDIS_COMMAND_FOR_STAND_ALONE_MODE_END,
//	        
//		REDIS_COMMAND_TO_SENTINEL_NODES_START,	// command to sentinel nodes
//        REDIS_COMMAND_SENTINEL_GET_MASTER_ADDR,
//        REDIS_COMMAND_INFO_REPLICATION,
//        REDIS_COMMAND_TO_SENTINEL_NODES_END,	// command to sentinel nodes
//	};
//};



enum RedisCommandType{
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
    REDIS_COMMAND_TO_SENTINEL_NODES_END,	// command to sentinel nodes
};



inline bool IsCommandReadType(RedisCommandType type)
{
	return static_cast<int>(type)>static_cast<int>(REDIS_COMMAND_READ_TYPE_START)  &&  static_cast<int>(type)<static_cast<int>(REDIS_COMMAND_READ_TYPE_END);
}

inline bool IsCommandWriteType(RedisCommandType type)
{
	return (int)type>(int)REDIS_COMMAND_WRITE_TYPE_START  &&  (int)type<(int)REDIS_COMMAND_WRITE_TYPE_END;
}

class RedisClient;

struct SwitchMasterThreadArgType
{
	RedisConnection* con;
	RedisClient* client;
};

struct CommandResult
{
	list<string>& members;
	DBSerialize* serial;
	int* count;
};

class RedisClient
{
public:
	enum ScanMode{
		SCAN_LOOP,
		SCAN_NOLOOP,
	};

	RedisClient();
	~RedisClient();
	bool init(const string& serverIp, uint32_t serverPort, uint32_t connectionNum, const string& passwd="");
	bool init(const REDIS_SERVER_LIST& clusterList, uint32_t connectionNum, const string& passwd="");
	bool init(const REDIS_SERVER_LIST& sentinelList, const string& masterName, uint32_t connectionNum, const string& passwd="");
	bool init(RedisMode redis_mode, const REDIS_SERVER_LIST& serverList, const string& masterName, uint32_t connectionNum, const string& passwd="", uint32_t keepaliveTime=-1);
    bool freeRedisClient();
	bool getSerial(const string& key, DBSerialize &serial);
	bool getSerialWithLock(const string& key, DBSerialize &serial, RedisLockInfo& lockInfo);
	bool find(const string& key);
	bool delKeys(const string & delKeys);
	bool setSerial(const string& key, const DBSerialize &serial);
	bool setSerial(const string& key, const string& serial);
	bool setSerialWithLock(const string& key, const DBSerialize &serial, RedisLockInfo& lockInfo);
	bool setSerialWithExpire(const string& key, const DBSerialize &serial, uint32_t expireTime);
	//need unwatch key.
	bool releaseLock(const string& key, RedisLockInfo& lockInfo);
	bool del(const string& key);
	bool setKeyExpireTime(const string& key, uint32_t expireTime);

	bool scanKeys(const string& queryKey, uint32_t count, list<string> &keys, ScanMode scanMode);
	//for query operation.
	bool getKeys(const string& queryKey, list<string>& keys);
//	bool getSerials(const string& key, map<string,RedisSerialize> &serials);
	
	bool zadd(const string& key,const string& member, int score);
	bool zrem(const string& key,const string& member);
	bool zincby(const string& key,const string& member,int increment);
	int zcount(const string& key,int start, int end);
	int zcard(const string& key);
    int dbsize();
	int zscore(const string& key,const string& member);
	bool zrangebyscore(const string& key,int start, int end, list<string>& members);
	bool zremrangebyscore(const string& key,int start, int end);
	
	bool sadd(const string& key, const string& member);
	bool srem(const string& key, const string& member);
	int scard(const string& key);
	bool sismember(const string& key, const string& member);
	bool smembers(const string& key, list<string>& members);

	bool isRunAsCluster() { return m_redisMode==CLUSTER_MODE; }

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
	bool Set(RedisConnection* con, const string & key, const DBSerialize& serial);
	bool Del(RedisConnection*, const string&);
	bool Sadd(RedisConnection*, const string& key, const string& member);
	bool Srem(RedisConnection*, const string& key, const string& member);

	bool StartSentinelHealthCheckTask();
	bool SubscribeSwitchMaster(RedisCluster* cluster);
	static void* SubscribeSwitchMasterThreadFun(void* arg);
	bool ParseSwithMasterMessage(const RedisReplyInfo& replyInfo, RedisServerInfo& masterAddr);
	bool DoSwitchMaster(const RedisServerInfo& masterAddr);

	void DoTestOfSentinelSlavesCommand();
	bool SentinelGetSlavesInfo(RedisCluster* cluster, vector<RedisServerInfo>& slavesAddr);
	bool ParseSentinelSlavesReply(const RedisReplyInfo& replyInfo, vector<RedisServerInfo>& slavesInfo);
	
private:
    bool getClusterIdFromRedirectReply(const string& redirectInfo, string& clusterId);
	bool getRedisClusterNodes();
	bool parseClusterInfo(RedisReplyInfo& replyInfo, REDIS_CLUSTER_MAP& clusterMap);
	bool parseOneCluster(const string& infoStr, RedisClusterInfo& clusterInfo);
	void parseSlotStr(string & slotStr, uint16_t &startSlotNum, uint16_t &stopSlotNum);
	bool initRedisCluster();
	bool freeRedisCluster();
	bool initRedisProxy();
	bool freeRedisProxy();
	
	bool checkIfNeedRedirect(RedisReplyInfo& replyInfo, bool &needRedirect, string& redirectInfo);

	bool parseGetSerialReply(RedisReplyInfo& replyInfo, DBSerialize &serial, bool &needRedirect, string& redirectInfo);
	bool parseSetSerialReply(RedisReplyInfo& replyInfo, bool &needRedirect, string& redirectInfo);
	//find and del and expire use this reply,for it reply is same.
	bool parseFindReply(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo);
	bool parseStatusResponseReply(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo);
	bool parseExecReply(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo);
	bool parseScanKeysReply(RedisReplyInfo & replyInfo, list<string>& keys, int& retCursor);
	bool parseGetKeysReply(RedisReplyInfo& replyInfo, list<string>& keys);
	
	void freeReplyInfo(RedisReplyInfo& replyInfo);
	void freeCommandList(list<RedisCmdParaInfo> &paraList);
	
	void fillCommandPara(const char* paraValue, int32_t paraLen, list<RedisCmdParaInfo> &paraList);
	void fillScanCommandPara(int cursor, const string& queryKey, int count, list<RedisCmdParaInfo>& paraList, int32_t& paraLen, ScanMode scanMode);
	
	//add for get one cluster info
	bool getRedisClusterInfo(string& clusterId, RedisClusterInfo& clusterInfo);
	void updateClusterCursor(const string& clusterId, int newcursor);
	bool getClusterIdBySlot(uint16_t slotNum, string& clusterId);

 	bool doRedisCommand(const string & key, int32_t commandLen, list<RedisCmdParaInfo> & commandList, RedisCommandType commandType);
 	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisCommandType commandType, DBSerialize* serial);
 	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisCommandType commandType, int* count);
	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisCommandType commandType, list<string>& members);
	bool doRedisCommand(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	bool doRedisCommandStandAlone(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	bool doRedisCommandMaster(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	bool ParseRedisReplyForStandAloneAndMasterMode(				RedisReplyInfo& replyInfo,
				bool& needRedirect,
				string& redirectInfo,
				const string& key,
				RedisCommandType commandType,
				list<string>& members,
				DBSerialize* serial, int* count);
	bool doRedisCommandCluster(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisCommandType commandType, list<string>& members, DBSerialize* serial, int* count);
	
	bool doRedisCommandWithLock(const string& key, int32_t commandLen, list<RedisCmdParaInfo> &commandList, int commandType, RedisLockInfo& lockInfo, bool getSerial = false, DBSerialize* serial = NULL);
	bool doMultiCommand(int32_t commandLen, list<RedisCmdParaInfo> &commandList, RedisConnection** conn);

	// for Transaction API
	bool doTransactionCommandInConnection(int32_t commandLen, list<RedisCmdParaInfo> &commandList, int commandType, RedisConnection* con);
	bool parseStatusResponseReply(RedisReplyInfo & replyInfo);
	bool parseQueuedResponseReply(RedisReplyInfo & replyInfo);
	bool parseExecReply(RedisReplyInfo & replyInfo);
	
	bool initSentinels();
	bool initMasterSlaves();
	bool SentinelGetMasterAddrByName(RedisCluster* cluster, RedisServerInfo& serverInfo);
	bool ParseSentinelGetMasterReply(const RedisReplyInfo& replyInfo, RedisServerInfo& serverInfo);
	bool MasterGetReplicationSlavesInfo(RedisCluster* cluster, vector<RedisServerInfo>& slaves);
	bool ParseInfoReplicationReply(const RedisReplyInfo& replyInfo, vector<RedisServerInfo>& slaves);
	bool freeSentinels();
	bool freeMasterSlaves();
	bool ParseSubsribeSwitchMasterReply(const RedisReplyInfo& replyInfo);
	static void* SentinelHealthCheckTask(void* arg);
	bool CheckMasterRole(const RedisServerInfo& masterAddr);
	bool SentinelReinit();
	void CheckSentinelGetMasterAddrByName();

	bool StartCheckClusterThread();
	void SignalToDoClusterNodes();
	static void* CheckClusterNodesThreadFunc(void* arg);
	void DoCheckClusterNodes();
//	bool CreateConnectionPool(map<string, RedisProxyInfo>::iterator& it);
	bool CreateConnectionPool(RedisProxyInfo& );

	bool AuthPasswd(const string& passwd, RedisCluster* cluster);
	bool ParseAuthReply(const RedisReplyInfo& replyInfo);
	
private:
	// for all RedisMode
	RedisMode m_redisMode;
	uint32_t m_connectionNum;
	uint32_t m_keepaliveTime;
	string m_passwd;
	bool m_connected;
		
	// for STAND_ALONE_OR_PROXY_MODE
    RedisProxyInfo m_redisProxy;
//	RWMutex m_rwProxyMutex;	
    
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
    queue<int> m_signalQueue;
    MutexLock m_lockSignalQueue;
    CondVar m_condSignalQueue;
    
    // for SENTINEL_MODE
    REDIS_SERVER_LIST m_sentinelList; // redis sentinel nodes addr
    string m_masterName;
    map<string, RedisProxyInfo> m_sentinelHandlers;
    RedisServerInfo m_initMasterAddr;
    map<string, RedisProxyInfo> m_dataNodes; // key: clusterId
    string m_masterClusterId;
    RWMutex m_rwMasterMutex;
    SwitchMasterThreadArgType m_threadArg;
    vector<pthread_t> m_subscribeThreadIdList;
    bool m_sentinelHealthCheckThreadStarted;
    pthread_t m_sentinelHealthCheckThreadId;
    RWMutex m_rwThreadIdMutex; // guard pthread_t above
};

#endif

