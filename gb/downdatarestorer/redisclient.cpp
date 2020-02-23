#include "redisclient.h"
#include"redisconnection.h"
#include"glog/logging.h"
#include"util.h"
#include"mutexlockguard.h"
//#include"redismonitor.h"
#include<algorithm>
#include<cassert>

RedisClient::RedisClient()
	:m_redisMode(STAND_ALONE_OR_PROXY_MODE),
	m_connectionNum(0),
	m_connectTimeout(-1),
	m_readTimeout(-1),
	m_passwd(),
	m_connected(false),
	m_redisProxy(),
	m_serverList(),
	m_slotMap(),
	m_rwSlotMutex(),
	m_clusterMap(),
	m_rwClusterMutex(),
	m_unusedHandlers(),
//	m_redisMonitor(NULL),
	m_checkClusterNodesThreadId(),
	m_checkClusterNodesThreadStarted(false),
	m_signalQueue(),
	m_lockSignalQueue(),
	m_condSignalQueue(&m_lockSignalQueue),
	m_sentinelList(),
	m_masterName(),
	m_sentinelHandlers(),
	m_initMasterAddr(),
	m_dataNodes(),
	m_masterClusterId(),
	m_rwMasterMutex(),
	m_threadArg(),
	m_subscribeThreadIdList(),
	m_sentinelHealthCheckThreadStarted(false),
	m_sentinelHealthCheckThreadId()
	
{
	LOG(INFO)<<"construct RedisClient ok";
}

RedisClient::~RedisClient()
{
    if(m_connected==true)
        freeRedisClient();
}

bool RedisClient::freeRedisClient()
{
    if(m_connected==false)
    {
		LOG(WARNING)<<"why RedisClient freeRedisClient called when m_connected is false";
        return false;
    }
    LOG(INFO)<<"free redisclients";

    m_passwd.clear();
    
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
	{
		freeRedisProxy();
	}
	else if(m_redisMode==CLUSTER_MODE)
	{
		freeRedisCluster();
//		if(m_redisMonitor)
//		{
//			m_redisMonitor->cancel();
//			delete m_redisMonitor;
//			m_redisMonitor=NULL;
//		}
	}
	else if(m_redisMode==SENTINEL_MODE)
	{
		freeSentinels();
		freeMasterSlaves();
	}
	
    m_connected=false;
    return true;
}

bool RedisClient::init(const string& serverIp, uint32_t serverPort, uint32_t connectionNum, uint32_t connectTimeout, uint32_t readTimeout, const string& passwd)
{	
	RedisServerInfo server(serverIp, serverPort);
	REDIS_SERVER_LIST serverList({server});
	return init(STAND_ALONE_OR_PROXY_MODE, serverList, "", connectionNum, connectTimeout, readTimeout, passwd);
}

bool RedisClient::init(const REDIS_SERVER_LIST& clusterList, uint32_t connectionNum, uint32_t connectTimeout, uint32_t readTimeout, const string& passwd)
{
	return init(CLUSTER_MODE, clusterList, "", connectionNum, connectTimeout, readTimeout, passwd);
}

bool RedisClient::init(const REDIS_SERVER_LIST& sentinelList, const string& masterName, uint32_t connectionNum, uint32_t connectTimeout, uint32_t readTimeout, const string& passwd)
{
	return init(SENTINEL_MODE, sentinelList, masterName, connectionNum, connectTimeout, readTimeout, passwd);
}

bool RedisClient::init(RedisMode redis_mode, const REDIS_SERVER_LIST& serverList, const string& masterName, uint32_t connectionNum, uint32_t connectTimeout, uint32_t readTimeout, const string& passwd)
{
    if(m_connected==true)
    {
		LOG(WARNING)<<"why RedisClient init called, when m_connected is true";
        return false;
    }

	m_redisMode=redis_mode;
	m_masterName=masterName;
	if(m_redisMode==SENTINEL_MODE)
		m_sentinelList=serverList;
	else
		m_serverList = serverList;
	m_connectionNum = connectionNum;
	m_connectTimeout=connectTimeout;
	m_readTimeout=readTimeout;
	m_passwd=passwd;
	
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
	{
		if (!initRedisProxy())
		{
			LOG(ERROR)<<"init redis proxy failed.";
			return false;
		}
		LOG(INFO)<<"init the redis proxy or redis server "<<m_redisProxy.connectIp<<", "<<m_redisProxy.connectPort<<" ok";
	}
	else if(m_redisMode==CLUSTER_MODE)
	{
		if (!getRedisClusterNodes())
		{
//			m_logger.error("get redis cluster info failed.please check redis config.");
			LOG(ERROR)<<"get redis cluster info failed.please check redis config.";
			return false;
		}

		if (!initRedisCluster())
		{
			LOG(ERROR)<<"init redis cluster failed.";
			return false;
		}
		
//		m_redisMonitor=new RedisMonitor(this);
//		if(m_redisMonitor==NULL)
//		{
//			LOG(WARNING)<<"create RedisMonitor failed";
//		}
//		else
//		{
//			m_redisMonitor->start(5);
//		}
		if(StartCheckClusterThread()==false)
		{
			LOG(ERROR)<<"start thread to check cluster nodes failed";
			return false;
		}
	}
	else if(m_redisMode==SENTINEL_MODE)
	{
		if(!initSentinels())
		{
			LOG(ERROR)<<"init sentinels failed";
			return false;
		}
		if(!initMasterSlaves())
		{
			LOG(ERROR)<<"init master and slaves connection failed";
			return false;
		}
		
		LOG(INFO)<<"init redis sentinels and data nodes connection ok";
	}
	else
	{
		LOG(ERROR)<<"unsupported redis mode "<<m_redisMode;
		return false;
	}
	
    m_connected=true;
	return true;
}

bool RedisClient::initRedisProxy()
{
//    WriteGuard guard(m_rwProxyMutex);
	m_redisProxy.connectIp=m_serverList.front().serverIp;
	m_redisProxy.connectPort=m_serverList.front().serverPort;
	m_redisProxy.proxyId=m_redisProxy.connectIp+":"+toStr(m_redisProxy.connectPort);
	m_redisProxy.connectionNum=m_connectionNum;
	m_redisProxy.connectTimeout=m_connectTimeout;
	m_redisProxy.readTimeout=m_readTimeout;

    if(m_redisProxy.clusterHandler != NULL)
    {
//        m_logger.error("why m_redisProxy.clusterHandler not NULL when initRedisProxy");
		LOG(WARNING)<<"why m_redisProxy.clusterHandler not NULL when initRedisProxy";
        freeRedisProxy();
    }
	m_redisProxy.clusterHandler = new RedisCluster();
    if(m_redisProxy.clusterHandler==NULL)
    {
        return false;
    }
	if (!m_redisProxy.clusterHandler->initConnectPool(m_redisProxy.connectIp, m_redisProxy.connectPort, m_redisProxy.connectionNum, m_redisProxy.connectTimeout, m_redisProxy.readTimeout))
	{
//		m_logger.warn("init proxy:[%s] connect pool failed.", m_redisProxy.proxyId.c_str());
		LOG(ERROR)<<"init redis-server:["<<m_redisProxy.proxyId<<"] connect pool failed.";
		delete m_redisProxy.clusterHandler; 
		m_redisProxy.clusterHandler=NULL;
		return false;
	}
//	m_logger.debug("init proxy:[%s] connect pool ok.", m_redisProxy.proxyId.c_str());
	LOG(INFO)<<"init redis-server:["<<m_redisProxy.proxyId<<"] connect pool ok.";

	if(m_passwd.empty()==false)
	{
		AuthPasswd(m_passwd, m_redisProxy.clusterHandler);
	}
	return true;
}


bool RedisClient::getRedisClusterNodes()
{
	assert(m_redisMode==CLUSTER_MODE);
	bool success = false;
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("cluster", 7, paraList);
	fillCommandPara("nodes", 5, paraList);
	paraLen += 30;
	REDIS_SERVER_LIST::iterator iter;
	for (iter = m_serverList.begin(); iter != m_serverList.end(); iter++)
	{
		RedisServerInfo serverInfo = (*iter);
		
		RedisConnection *connection = new RedisConnection(serverInfo.serverIp, serverInfo.serverPort, m_connectTimeout, m_readTimeout);
		if (!connection->connect())
		{
//			m_logger.warn("connect to serverIp:[%s] serverPort:[%d] failed.", serverInfo.serverIp.c_str(), serverInfo.serverPort);
			LOG(WARNING)<<"connect to serverIp:["<<serverInfo.serverIp<<"] serverPort:["<<serverInfo.serverPort<<"] failed.";
			delete connection;
			continue;
		}
		//send cluster nodes.
		RedisReplyInfo replyInfo;
		if (!connection->doRedisCommand(paraList, paraLen, replyInfo))
		{
//			m_logger.warn("do get cluster nodes failed.");
			LOG(WARNING)<<"do get cluster nodes failed.";
			connection->close();
			delete connection;
			freeReplyInfo(replyInfo);
			freeCommandList(paraList);
			continue;
		}
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
		{
//			m_logger.warn("recv redis error response:[%s].", replyInfo.resultString.c_str());
			LOG(ERROR)<<"recv redis error response:["<<replyInfo.resultString<<"].";
			connection->close();
			delete connection;
			freeReplyInfo(replyInfo);
			freeCommandList(paraList);
			return false;
		}
		if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
		{
//			m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
			LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
			connection->close();
			delete connection;
			freeReplyInfo(replyInfo);
			freeCommandList(paraList);
			return false;
		}
		//check if master,slave parameter may be not newest.
		list<ReplyArrayInfo>::iterator arrayIter = replyInfo.arrayList.begin();
//		string str = (*arrayIter).arrayValue;
//		if (str.find("myself,slave") != string::npos)
//		{
//			m_logger.info("the redis cluster:[%s:%d] is slave. not use it reply,look for master.", serverInfo.serverIp.c_str(), serverInfo.serverPort);
//			freeReplyInfo(replyInfo);
//			connection->close();
//			delete connection;
//			continue;
//		}
		REDIS_CLUSTER_MAP clusterMap;
		if (!parseClusterInfo(replyInfo, clusterMap))
		{
//			m_logger.error("parse cluster info from redis reply failed.");
			LOG(ERROR)<<"parse cluster info from redis reply failed.";
			connection->close();
			delete connection;
			freeReplyInfo(replyInfo);
			freeCommandList(paraList);
			return false;
		}
		{
			WriteGuard guard(m_rwClusterMutex);
			m_clusterMap = clusterMap;
		}
		freeReplyInfo(replyInfo);
		connection->close();
		delete connection;
		success = true;
		break;
	}	
	freeCommandList(paraList);
	return success;
}

bool RedisClient::initRedisCluster()
{
	WriteGuard guard(m_rwClusterMutex);
	REDIS_CLUSTER_MAP::iterator clusterIter;
	for (clusterIter = m_clusterMap.begin(); clusterIter != m_clusterMap.end(); clusterIter++)
	{
		((*clusterIter).second).clusterHandler = new RedisCluster();
		if (!((*clusterIter).second).clusterHandler->initConnectPool(((*clusterIter).second).connectIp, ((*clusterIter).second).connectPort, ((*clusterIter).second).connectionNum, ((*clusterIter).second).connectTimeout, ((*clusterIter).second).readTimeout))
		{
//			m_logger.warn("init cluster:[%s] connect pool failed.", (*clusterIter).first.c_str());
			LOG(ERROR)<<"init cluster:["<<(*clusterIter).first<<"] connect pool failed.";
			delete ((*clusterIter).second).clusterHandler;
			((*clusterIter).second).clusterHandler=NULL;
			return false;
		}
		//fill slot map
		if (((*clusterIter).second).isMaster)
		{
			WriteGuard guard(m_rwSlotMutex);
			map<uint16_t,uint16_t>::iterator iter;
			for(iter = ((*clusterIter).second).slotMap.begin(); iter != ((*clusterIter).second).slotMap.end(); iter++)
			{
				uint16_t startSlotNum = (*iter).first;
				uint16_t stopSlotNum = (*iter).second;
				for(int i = startSlotNum; i <= stopSlotNum; i++)
				{
					m_slotMap[i] = (*clusterIter).first;
				}
			}
		}
		if(m_passwd.empty()==false)
		{
			AuthPasswd(m_passwd, ((*clusterIter).second).clusterHandler);
		}
	}
	return true;
}

bool RedisClient::initSentinels()
{
	bool masterNodeGot=false;
	int aliveSentinel=0;
	bool switchMasterSubsribed=false;
	for(REDIS_SERVER_LIST::const_iterator it=m_sentinelList.begin(); it!=m_sentinelList.end(); ++it)
	{
		RedisProxyInfo sentinalHandler;
		sentinalHandler.connectIp=it->serverIp;
		sentinalHandler.connectPort=it->serverPort;
		sentinalHandler.proxyId=it->serverIp+":"+toStr(sentinalHandler.connectPort);
		sentinalHandler.isAlived=false;
		sentinalHandler.subscribed=false;
		sentinalHandler.clusterHandler=new RedisCluster;		
		if(sentinalHandler.clusterHandler==NULL)
		{
			LOG(ERROR)<<"new RedisCluster failed";	
		}
		else
		{
			// one for control and check, like ckquorum; 
			// one for subscribe +switch-master;
			int connection_num_to_sentinel=2;
			if (!sentinalHandler.clusterHandler->initConnectPool(sentinalHandler.connectIp, sentinalHandler.connectPort, connection_num_to_sentinel, m_connectTimeout, m_readTimeout))
			{
				LOG(ERROR)<<"init sentinel:["<<sentinalHandler.proxyId<<"] connect pool failed.";
			}
			else
			{
				sentinalHandler.isAlived=true;
				aliveSentinel++;
			}
		}		

		m_sentinelHandlers[sentinalHandler.proxyId]=sentinalHandler;

		if(!sentinalHandler.isAlived)
			continue;

		if(!SubscribeSwitchMaster(sentinalHandler.clusterHandler))
		{
			LOG(ERROR)<<"sentinel "<<sentinalHandler.proxyId<<" subscribe +switch-master failed";
		}
		else
		{
			switchMasterSubsribed=true;
//			sentinalHandler.subscribed=true;
			m_sentinelHandlers[sentinalHandler.proxyId].subscribed=true;
		}

		if(!masterNodeGot)
		{
			if(SentinelGetMasterAddrByName(sentinalHandler.clusterHandler, m_initMasterAddr)  
				&&  CheckMasterRole(m_initMasterAddr))
			{
				masterNodeGot=true;
			}
		}
	}
	LOG(INFO)<<"config sentinel size is "<<m_sentinelList.size()<<", connected sentinel is "<<aliveSentinel;

	if(!StartSentinelHealthCheckTask())
	{
		LOG(WARNING)<<"fail to start sentinel health check task";
	}
	
	if(!masterNodeGot)
	{
		LOG(ERROR)<<"get master addr failed";
		goto CLEAR_SENTINELS;
	}

	if(!switchMasterSubsribed)
	{
		LOG(ERROR)<<"subscribe +switch-master failed";
		goto CLEAR_SENTINELS;
	}

	return true;

CLEAR_SENTINELS:
	for(map<string, RedisProxyInfo>::iterator it=m_sentinelHandlers.begin(); it!=m_sentinelHandlers.end(); ++it)
	{
		if(it->second.clusterHandler)
		{
			it->second.clusterHandler->freeConnectPool();
			delete it->second.clusterHandler;
		}
	}
	m_sentinelHandlers.clear();
	if(m_sentinelHealthCheckThreadStarted)
		pthread_cancel(m_sentinelHealthCheckThreadId);
	{
		WriteGuard guard(m_rwThreadIdMutex);
		for(size_t i=0; i<m_subscribeThreadIdList.size(); i++)
		{
			pthread_cancel(m_subscribeThreadIdList[i]);
		}
		m_subscribeThreadIdList.clear();
	}
	return false;
}

bool RedisClient::CheckMasterRole(const RedisServerInfo& masterAddr)
{
	return true;
}

bool RedisClient::StartSentinelHealthCheckTask()
{
	if(m_sentinelHealthCheckThreadStarted)
		return true;
	int ret=pthread_create(&m_sentinelHealthCheckThreadId, NULL, SentinelHealthCheckTask, this);
	if(ret)
	{
		LOG(ERROR)<<"start health check thread failed";
		return false;
	}
	m_sentinelHealthCheckThreadStarted=true;;
	return true;
}

bool RedisClient::SentinelReinit()
{
	bool sentinel_reinit_work_all_done=true;
	for(map<string, RedisProxyInfo>::iterator it=m_sentinelHandlers.begin(); it!=m_sentinelHandlers.end(); ++it)
	{
		if(it->second.isAlived==false)
		{
			//
			if(it->second.clusterHandler==NULL)
				it->second.clusterHandler=new RedisCluster;
			if(it->second.clusterHandler==NULL)
			{
				LOG(ERROR)<<"new RedisCluster failed";	
				sentinel_reinit_work_all_done=false;
			}
			else
			{
				int connection_num_to_sentinel=2;
				if (!it->second.clusterHandler->initConnectPool(it->second.connectIp, it->second.connectPort, connection_num_to_sentinel, m_connectTimeout, m_readTimeout))
				{
					LOG(ERROR)<<"init sentinel:["<<it->second.proxyId<<"] connect pool failed.";
					sentinel_reinit_work_all_done=false;
				}
				else
				{
					it->second.isAlived=true;
					LOG(INFO)<<"sentinel now online: "<<it->second.connectIp<<", "<<it->second.connectPort;
				}
			}				
		}
		
		if(it->second.isAlived  &&  !it->second.subscribed)
		{
			if(!SubscribeSwitchMaster(it->second.clusterHandler))
			{
				LOG(ERROR)<<"sentinel "<<it->second.proxyId<<" subscribe +switch-master failed";
				sentinel_reinit_work_all_done=false;
			}
			else
			{
				it->second.subscribed=true;
			}
		}
	}

	return sentinel_reinit_work_all_done;
}

void* RedisClient::SentinelHealthCheckTask(void* arg)
{
	LOG(INFO)<<"sentinel health check task start";

	RedisClient* client=(RedisClient*)arg;

	bool sentinel_reinit_work_all_done=false;
	
	while(true)
	{
//		bool sentinel_reinit_work_all_done=true;
//		for(map<string, RedisProxyInfo>::iterator it=client->m_sentinelHandlers.begin(); it!=client->m_sentinelHandlers.end(); ++it)
//		{
//			if(it->second.isAlived==false)
//			{
//				//
//				if(it->second.clusterHandler==NULL)
//					it->second.clusterHandler=new RedisCluster;
//				if(it->second.clusterHandler==NULL)
//				{
//					LOG(ERROR)<<"new RedisCluster failed";	
//					sentinel_reinit_work_all_done=false;
//				}
//				else
//				{
//					int connection_num_to_sentinel=2;
//					if (!it->second.clusterHandler->initConnectPool(it->second.connectIp, it->second.connectPort, connection_num_to_sentinel, client->m_connectTimeout, client->m_readTimeout))
//					{
//						LOG(ERROR)<<"init sentinel:["<<it->second.proxyId<<"] connect pool failed.";
//						sentinel_reinit_work_all_done=false;
//					}
//					else
//					{
//						it->second.isAlived=true;
//						LOG(INFO)<<"sentinel now online: "<<it->second.connectIp<<", "<<it->second.connectPort;
//					}
//				}				
//			}
//			
//			if(it->second.isAlived  &&  !it->second.subscribed)
//			{
//				if(!client->SubscribeSwitchMaster(it->second.clusterHandler))
//				{
//					LOG(ERROR)<<"sentinel "<<it->second.proxyId<<" subscribe +switch-master failed";
//					sentinel_reinit_work_all_done=false;
//				}
//				else
//				{
//					it->second.subscribed=true;
//				}
//			}
//		}
//
//		if(sentinel_reinit_work_all_done)
//			break;

		if(sentinel_reinit_work_all_done==false)
			sentinel_reinit_work_all_done=client->SentinelReinit();
		else
			break;

//		client->CheckSentinelGetMasterAddrByName();
		
		sleep(60);
	}

	LOG(INFO)<<"sentinel health check task finish, exit";
	client->m_sentinelHealthCheckThreadStarted=false;
	return 0;
}

void RedisClient::CheckSentinelGetMasterAddrByName()
{
	// TODO 1,lock guard, 2, check connection pool
	
//	for(map<string, RedisProxyInfo>::iterator it=m_sentinelHandlers.begin(); it!=m_sentinelHandlers.end(); ++it)
//	{
//		RedisServerInfo masterAddr;
//		if(SentinelGetMasterAddrByName(it->second.clusterHandler, masterAddr)  
//				&&  CheckMasterRole(masterAddr))
//		{
//			
//		}
//	}
}

bool RedisClient::SubscribeSwitchMaster(RedisCluster* cluster)
{
	// allocate and occupy one RedisConnection from pool
	RedisConnection* con=cluster->getAvailableConnection();
	if(con==NULL)
	{
		LOG(ERROR)<<"cannot acquire a redis connection";
		return false;
	}
	con->SetCanRelease(false);

	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	string cmd1="subscribe";
	fillCommandPara(cmd1.c_str(), cmd1.size(), paraList);
	paraLen += cmd1.size()+10;
	string cmd2="+switch-master";
	fillCommandPara(cmd2.c_str(), cmd2.size(), paraList);
	paraLen += cmd2.size()+10;

		
	RedisReplyInfo replyInfo;
	bool success=con->doRedisCommand(paraList, paraLen, replyInfo);
	if(!success)
	{
		LOG(ERROR)<<"do cmd subscribe +switch-master failed";
		return false;
	}

	if(!ParseSubsribeSwitchMasterReply(replyInfo))
	{
		LOG(INFO)<<"subscribe +switch-master failed";
		return false;
	}

	pthread_t thread_id;
	m_threadArg.con=con;
	m_threadArg.client=this;
	if(pthread_create(&thread_id, NULL, SubscribeSwitchMasterThreadFun, &m_threadArg))
	{
		PLOG(ERROR)<<"create subscribe +switch-master thread failed";
		return false;
	}
	else
	{
		WriteGuard guard(m_rwThreadIdMutex);
		m_subscribeThreadIdList.push_back(thread_id);
		return true;
	}
	
}

void* RedisClient::SubscribeSwitchMasterThreadFun(void* arg)
{
	SwitchMasterThreadArgType* tmp=(SwitchMasterThreadArgType*)arg;
	
	RedisClient* client=tmp->client;
	RedisConnection* con=tmp->con;
	LOG(INFO)<<"subscribe +switch-master thread start, sentinel addr: "<<con->GetServerIp()<<", "<<con->GetServerPort();

	int handler;
	if(!con->ListenMessage(handler))
	{
		LOG(ERROR)<<"fail to listen switch-master mesages";
		return 0;
	}
	while(true)
	{
		LOG(INFO)<<"redisclient: waiting +switch-master channel message now";
		RedisReplyInfo replyInfo;
		if(con->WaitMessage(handler))
		{
			con->recv(replyInfo);
		}
		else
		{
			continue;
		}

		RedisServerInfo masterAddr;		
		if(!client->ParseSwithMasterMessage(replyInfo, masterAddr))
		{
			LOG(ERROR)<<"get new master failed";
			continue;
		}

		client->DoSwitchMaster(masterAddr);
	}

	con->StopListen(handler);
	
	return NULL;
}

bool RedisClient::ParseSwithMasterMessage(const RedisReplyInfo& replyInfo, RedisServerInfo& masterAddr)
{
	LOG(INFO)<<"channel +switch-master has message, replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
	{
		LOG(INFO)<<"get error reply.";
		return false;
	}

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_ARRAY)
	{
		LOG(INFO)<<"get string reply: "<<replyInfo.resultString;
		return false;
	}

	if(replyInfo.arrayList.size()!=3)
	{
		return false;
	}
	int i=0;
	list<ReplyArrayInfo>::const_iterator arrayIter;
	for (arrayIter = replyInfo.arrayList.begin(); arrayIter != replyInfo.arrayList.end(); arrayIter++, i++)
	{
		LOG(INFO)<<"arrayList has replyType "<<(*arrayIter).replyType<<", arrayValue "<<arrayIter->arrayValue<<", arrayLen "<<arrayIter->arrayLen;
		
		if ((*arrayIter).replyType == RedisReplyType::REDIS_REPLY_STRING)
		{
			string message = (*arrayIter).arrayValue;
//			LOG(INFO)<<message;
			//  "mymaster 192.168.12.59 7100 192.168.12.59 7102"
			if(i==2)
			{
				string::size_type pos=message.rfind(' ');
				if(pos==string::npos)
					return false;
				masterAddr.serverPort=atoi(message.substr(pos+1).c_str());
				
				string::size_type pos2=pos-1;
				pos=message.rfind(' ', pos2);
				if(pos==string::npos)
					return false;
				masterAddr.serverIp=message.substr(pos+1, pos2-pos);
				
				LOG(INFO)<<"get new master addr: "<<masterAddr.serverIp<<", "<<masterAddr.serverPort;
				return true;
			}
		}
	}
	return false;
}

bool RedisClient::ParseSubsribeSwitchMasterReply(const RedisReplyInfo& replyInfo)
{
	LOG(INFO)<<"subscribe +switch-master has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
	{
		LOG(INFO)<<"get error reply.";
		return false;
	}

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_ARRAY)
	{
		LOG(INFO)<<"get string reply: "<<replyInfo.resultString;
		return false;
	}

	if(replyInfo.arrayList.size()!=3)
	{
		return false;
	}
	int i=0;
	list<ReplyArrayInfo>::const_iterator arrayIter;
	for (arrayIter = replyInfo.arrayList.begin(); arrayIter != replyInfo.arrayList.end(); arrayIter++, i++)
	{
		LOG(INFO)<<"arrayList has replyType "<<(*arrayIter).replyType<<", arrayValue "<<arrayIter->arrayValue<<", arrayLen "<<arrayIter->arrayLen;
		
		if ((*arrayIter).replyType == RedisReplyType::REDIS_REPLY_STRING)
		{
			string message = (*arrayIter).arrayValue;
//			LOG(INFO)<<message;
			if(i==2   &&  message!=":1")
			{
				LOG(ERROR)<<"subscribe +switch-master return error: "<<message;
				return false;
			}
		}
	}
	return true;
}

bool RedisClient::DoSwitchMaster(const RedisServerInfo& masterAddr)
{
	WriteGuard guard(m_rwMasterMutex);
	m_masterClusterId=masterAddr.serverIp+":"+toStr(masterAddr.serverPort);

	// TODO
	if(m_dataNodes.count(m_masterClusterId)==0)
	{
		LOG(ERROR)<<"no info of new master: "<<m_masterClusterId;
		return false;
	}

	RedisProxyInfo& masterNode=m_dataNodes[m_masterClusterId];
	if(masterNode.isAlived)
	{
		LOG(INFO)<<"master "<<m_masterClusterId<<" has connected";
		return true;
	}

	if(masterNode.clusterHandler==NULL)
	{
		masterNode.clusterHandler=new RedisCluster();
		if(masterNode.clusterHandler==NULL)
			return false;
	}

	if(!masterNode.clusterHandler->initConnectPool(masterNode.connectIp, masterNode.connectPort, m_connectionNum, m_connectTimeout, m_readTimeout))
	{
		LOG(ERROR)<<"init master connection pool failed";
		return false;
	}

	if(m_passwd.empty()==false)
	{
		AuthPasswd(m_passwd, masterNode.clusterHandler);
	}
	masterNode.isAlived=true;
	return true;
}

bool RedisClient::initMasterSlaves()
{
	vector<RedisServerInfo> slavesAddr;
	RedisProxyInfo masterNode;
	masterNode.connectIp=m_initMasterAddr.serverIp;
	masterNode.connectPort=m_initMasterAddr.serverPort;
	masterNode.proxyId=masterNode.connectIp+":"+toStr(masterNode.connectPort);
	masterNode.isAlived=false;
	masterNode.clusterHandler=new RedisCluster;
	if(masterNode.clusterHandler==NULL)
	{
		return false;
	}
	if(!masterNode.clusterHandler->initConnectPool(masterNode.connectIp, masterNode.connectPort, m_connectionNum, m_connectTimeout, m_readTimeout))
	{
		LOG(ERROR)<<"init master node connection pool failed";
		goto CLEAR_DATANODES;
	}
	if(m_passwd.empty()==false)
	{
		AuthPasswd(m_passwd, masterNode.clusterHandler);
	}

	m_masterClusterId=masterNode.proxyId;
	masterNode.isAlived=true;
	m_dataNodes[masterNode.proxyId]=masterNode;

//	if(!SentinelGetSlavesInfo(slavesAddr))
	if(!MasterGetReplicationSlavesInfo(masterNode.clusterHandler, slavesAddr))
	{
		LOG(WARNING)<<"get slave nodes failed";
		goto CLEAR_DATANODES;
	}

	LOG(INFO)<<"slaves count is "<<slavesAddr.size();
	for(size_t i=0; i<slavesAddr.size(); i++)
	{
		RedisProxyInfo slave_node;
		slave_node.connectIp=slavesAddr[i].serverIp;
		slave_node.connectPort=slavesAddr[i].serverPort;
		slave_node.proxyId=slave_node.connectIp+":"+toStr(slave_node.connectPort);
		slave_node.isAlived=false;
		slave_node.clusterHandler=new RedisCluster;
		if(slave_node.clusterHandler==NULL)
		{
			LOG(ERROR)<<"new slave RedisCluster failed";
		}
		else if(!slave_node.clusterHandler->initConnectPool(slave_node.connectIp, slave_node.connectPort, m_connectionNum, m_connectTimeout, m_readTimeout))
		{
			LOG(WARNING)<<"init slave connection pool failed";
		}
		else
		{
			slave_node.isAlived=true;

			if(m_passwd.empty()==false)
			{
				AuthPasswd(m_passwd, slave_node.clusterHandler);
			}
		}

		m_dataNodes[slave_node.proxyId]=slave_node;
	}
	return true;

CLEAR_DATANODES:
	for(map<string, RedisProxyInfo>::iterator it=m_dataNodes.begin(); it!=m_dataNodes.end(); ++it)
	{
		if(it->second.clusterHandler)
		{
			it->second.clusterHandler->freeConnectPool();
			delete it->second.clusterHandler;
		}
	}
	m_dataNodes.clear();
	return false;
}

bool RedisClient::freeRedisProxy()
{
//	WriteGuard guard(m_rwProxyMutex);
	if (m_redisProxy.clusterHandler != NULL)
	{
//        m_logger.info("freeRedisProxy call freeConnectionPool");
		LOG(INFO)<<"freeRedisProxy call freeConnectionPool";
		m_redisProxy.clusterHandler->freeConnectPool();
        delete m_redisProxy.clusterHandler;
		m_redisProxy.clusterHandler = NULL;
	}

	return true;
}

bool RedisClient::freeRedisCluster()
{
	WriteGuard guard(m_rwClusterMutex);
	REDIS_CLUSTER_MAP::iterator clusterIter;
	for (clusterIter = m_clusterMap.begin(); clusterIter != m_clusterMap.end(); clusterIter++)
	{
		RedisClusterInfo clusterInfo = (*clusterIter).second;
		if (clusterInfo.clusterHandler != NULL)
		{
			clusterInfo.clusterHandler->freeConnectPool();
			delete clusterInfo.clusterHandler;
			clusterInfo.clusterHandler = NULL;
		}
	}
	m_clusterMap.clear();
	if(m_checkClusterNodesThreadStarted)
	{
		pthread_cancel(m_checkClusterNodesThreadId);
		m_checkClusterNodesThreadStarted=false;
	}
	return true;
}

bool RedisClient::freeSentinels()
{
	for(map<string, RedisProxyInfo>::iterator it=m_sentinelHandlers.begin(); it!=m_sentinelHandlers.end(); ++it)
	{
		if(it->second.clusterHandler)
		{
			it->second.clusterHandler->freeConnectPool();
			delete it->second.clusterHandler;
			it->second.clusterHandler=NULL;
		}
	}
	m_sentinelHandlers.clear();

	if(m_sentinelHealthCheckThreadStarted)
		pthread_cancel(m_sentinelHealthCheckThreadId);
	{
		WriteGuard guard(m_rwThreadIdMutex);
		for(size_t i=0; i<m_subscribeThreadIdList.size(); i++)
		{
			pthread_cancel(m_subscribeThreadIdList[i]);
		}
		m_subscribeThreadIdList.clear();
	}
	return true;
}

bool RedisClient::freeMasterSlaves()
{
	for(map<string, RedisProxyInfo>::iterator it=m_dataNodes.begin(); it!=m_dataNodes.end(); ++it)
	{
		if(it->second.clusterHandler)
		{
			if(it->second.isAlived)
				it->second.clusterHandler->freeConnectPool();
			delete it->second.clusterHandler;
			it->second.clusterHandler=NULL;
		}
	}
	m_dataNodes.clear();
	return true;
}

bool RedisClient::SentinelGetMasterAddrByName(RedisCluster* cluster, RedisServerInfo& serverInfo)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	string cmd1="sentinel";
	fillCommandPara(cmd1.c_str(), cmd1.size(), paraList);
	paraLen += cmd1.size()+10;
	string cmd2="get-master-addr-by-name";
	fillCommandPara(cmd2.c_str(), cmd2.size(), paraList);
	paraLen += cmd2.size()+10;
	fillCommandPara(m_masterName.c_str(), m_masterName.length(), paraList);
	paraLen += m_masterName.length() + 10;
		
	RedisReplyInfo replyInfo;
	bool success=cluster->doRedisCommand(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"sentinel get-master-addr-by-name failed";
		return false;
	}

	if(!ParseSentinelGetMasterReply(replyInfo, serverInfo))
	{
		freeReplyInfo(replyInfo);
		return false;
	}
	else
	{
		freeReplyInfo(replyInfo);
		return true;
	}
}

bool RedisClient::ParseSentinelGetMasterReply(const RedisReplyInfo& replyInfo, RedisServerInfo& serverInfo)
{
	LOG(INFO)<<"sentinel get-master-addr-by-name command has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
	{
		LOG(INFO)<<"get empty list or set.";
		return false;
	}

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_ARRAY)
	{
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}

	if(replyInfo.arrayList.size()!=2)
	{
		LOG(ERROR)<<"wrong result";
		return false;
	}
	
	list<ReplyArrayInfo>::const_iterator arrayIter=replyInfo.arrayList.begin();
	serverInfo.serverIp=arrayIter->arrayValue;

	arrayIter++;
	LOG(INFO)<<"arrayList replyType "<<(*arrayIter).replyType<<", arrayValue "<<arrayIter->arrayValue<<", arrayLen "<<arrayIter->arrayLen;
	serverInfo.serverPort=atoi(arrayIter->arrayValue);

	LOG(INFO)<<"sentinel get-master-addr-by-name ok: "<<serverInfo.serverIp<<", "<<serverInfo.serverPort;
	return true;
}

void RedisClient::DoTestOfSentinelSlavesCommand()
{
	for(map<string, RedisProxyInfo>::iterator it=m_sentinelHandlers.begin(); it!=m_sentinelHandlers.end(); ++it)
	{
		if(it->second.isAlived)
		{
			vector<RedisServerInfo> slavesAddr;
			if(SentinelGetSlavesInfo(it->second.clusterHandler, slavesAddr))
			{
				LOG(INFO)<<"norah do test ok";
				return;
			}
			else
			{
				LOG(INFO)<<"norah do test failed";
			}
		}
	}
}

bool RedisClient::DoSaddWithParseEnhance(const string& setKey, const string& setMember)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="sadd";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(setKey.c_str(), setKey.length(), paraList);
    paraLen += setKey.length() + 11;
    fillCommandPara(setMember.c_str(), setMember.length(), paraList);
    paraLen += setMember.length() + 11;

	RedisCluster* cluster;
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
		cluster=m_redisProxy.clusterHandler;
	else if(m_redisMode==SENTINEL_MODE)
		cluster=m_dataNodes[m_masterClusterId].clusterHandler;
	else
		return false;

	if(cluster==NULL)
		return false;

    CommonReplyInfo replyInfo;
	bool success=cluster->testDoCommandWithParseEnhance(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"sadd "<<setKey<<" "<<setMember<<" failed";
		return false;
	}

	freeReplyInfo(replyInfo);
	return true;
}

bool RedisClient::DoGetWithParseEnhance(const string& key, string& value)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="get";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 11;

	RedisCluster* cluster;
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
		cluster=m_redisProxy.clusterHandler;
	else if(m_redisMode==SENTINEL_MODE)
		cluster=m_dataNodes[m_masterClusterId].clusterHandler;
	else
		return false;

	if(cluster==NULL)
		return false;

    CommonReplyInfo replyInfo;
	bool success=cluster->testDoCommandWithParseEnhance(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"get "<<key<<" failed";
		return false;
	}

	freeReplyInfo(replyInfo);
	return true;
}


bool RedisClient::DoSetWithParseEnhance(const string& key, const string& value)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="set";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 11;
    fillCommandPara(value.c_str(), value.length(), paraList);
    paraLen += value.length() + 11;

	RedisCluster* cluster;
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
		cluster=m_redisProxy.clusterHandler;
	else if(m_redisMode==SENTINEL_MODE)
		cluster=m_dataNodes[m_masterClusterId].clusterHandler;
	else
		return false;

	if(cluster==NULL)
		return false;

    CommonReplyInfo replyInfo;
	bool success=cluster->testDoCommandWithParseEnhance(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"set "<<key<<" "<<value<<" failed";
		return false;
	}

	freeReplyInfo(replyInfo);
	return true;
}

bool RedisClient::DoWrongCmdWithParseEnhance(const string& wrongCmd)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara(wrongCmd.c_str(), wrongCmd.length(), paraList);
    paraLen += wrongCmd.length() + 11;

	RedisCluster* cluster;
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
		cluster=m_redisProxy.clusterHandler;
	else if(m_redisMode==SENTINEL_MODE)
		cluster=m_dataNodes[m_masterClusterId].clusterHandler;
	else
		return false;

	if(cluster==NULL)
		return false;

    CommonReplyInfo replyInfo;
	bool success=cluster->testDoCommandWithParseEnhance(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"do cmd "<<wrongCmd<<" failed";
		return false;
	}

	freeReplyInfo(replyInfo);
	return true;
}

bool RedisClient::DoScanWithParseEnhance()
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    // scan 0
    string scan="scan";
    fillCommandPara(scan.c_str(), scan.length(), paraList);
    paraLen += scan.length() + 11;
    string arg="0";
    fillCommandPara(arg.c_str(), arg.length(), paraList);
    paraLen += arg.length() + 11;

	RedisCluster* cluster;
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
		cluster=m_redisProxy.clusterHandler;
	else if(m_redisMode==SENTINEL_MODE)
		cluster=m_dataNodes[m_masterClusterId].clusterHandler;
	else
		return false;

	if(cluster==NULL)
		return false;

    CommonReplyInfo replyInfo;
	bool success=cluster->testDoCommandWithParseEnhance(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"do scan 0 failed";
		return false;
	}

	freeReplyInfo(replyInfo);
	return true;
}


bool RedisClient::DoSmembersWithParseEnhance(const string& setKey, list<string>& members)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="smembers";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(setKey.c_str(), setKey.length(), paraList);
    paraLen += setKey.length() + 11;

	RedisCluster* cluster;
	if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
		cluster=m_redisProxy.clusterHandler;
	else if(m_redisMode==SENTINEL_MODE)
		cluster=m_dataNodes[m_masterClusterId].clusterHandler;
	else
		return false;

	if(cluster==NULL)
		return false;

    CommonReplyInfo replyInfo;
	bool success=cluster->testDoCommandWithParseEnhance(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"sadd "<<setKey<<" failed";
		return false;
	}

	ParseSmembersFromParseEnhance(replyInfo);
	freeReplyInfo(replyInfo);
	return true;
}

bool RedisClient::ParseSmembersFromParseEnhance(const CommonReplyInfo& replyInfo)
{
	return true;	
}

bool RedisClient::SentinelGetSlavesInfo(RedisCluster* cluster, vector<RedisServerInfo>& slavesAddr)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	string cmd1="sentinel";
	fillCommandPara(cmd1.c_str(), cmd1.size(), paraList);
	paraLen += cmd1.size()+10;
	string cmd2="slaves";
	fillCommandPara(cmd2.c_str(), cmd2.size(), paraList);
	paraLen += cmd2.size()+10;
	fillCommandPara(m_masterName.c_str(), m_masterName.length(), paraList);
	paraLen += m_masterName.length() + 20;
		
	CommonReplyInfo replyInfo;
	bool success=cluster->testDoCommandWithParseEnhance(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"sentinel slaves failed";
		return false;
	}

	if(!ParseSentinelSlavesReply(replyInfo, slavesAddr))
	{
		freeReplyInfo(replyInfo);
		return false;
	}
	else
	{
		freeReplyInfo(replyInfo);
		return true;
	}
}

bool RedisClient::ParseSentinelSlavesReply(const CommonReplyInfo& replyInfo, vector<RedisServerInfo>& slavesInfo)
{
	LOG(INFO)<<"sentinel slalves command has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
	{
		LOG(INFO)<<"get empty list or set.";
		return false;
	}

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_MULTI_ARRRY)
	{
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}

	string ip="ip";
	string port="port";
	for(size_t i=0; i<replyInfo.arrays.size(); i++)
	{
		LOG(INFO)<<"get the "<<i<<" Array, size is "<<replyInfo.arrays[i].size();
		for(size_t j=0; j<replyInfo.arrays[i].size(); j++)
		{
//			LOG(INFO)<<"replyType "<<replyInfo.arrays[i][j].replyType<<", arrayValue "<<replyInfo.arrays[i][j].arrayValue<<", arrayLen "<<replyInfo.arrays[i][j].arrayLen;
			
			if (replyInfo.arrays[i][j].replyType == RedisReplyType::REDIS_REPLY_STRING)
			{
				string message = replyInfo.arrays[i][j].arrayValue;
//				LOG(INFO)<<"["<<message<<"]";

				if(strcmp(message.c_str(), ip.c_str())==0  && j+3<replyInfo.arrays[i].size())
				{
					string message2 = replyInfo.arrays[i][j+2].arrayValue;
					if(strcmp(message2.c_str(), port.c_str())!=0)
						break;
					RedisServerInfo addr;
					addr.serverIp=replyInfo.arrays[i][j+1].arrayValue;
					addr.serverPort=atoi(replyInfo.arrays[i][j+3].arrayValue);
					LOG(INFO)<<"get slave addr: "<<addr.serverIp<<", "<<addr.serverPort;
					slavesInfo.push_back(addr);
					break;
				}
			}
		}
	}
	
	return true;
}

bool RedisClient::MasterGetReplicationSlavesInfo(RedisCluster* cluster, vector<RedisServerInfo>& slaves)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	string cmd="info";
	fillCommandPara(cmd.c_str(), cmd.size(), paraList);
	paraLen += cmd.size()+10;
	cmd="replication";
	fillCommandPara(cmd.c_str(), cmd.size(), paraList);
	paraLen += cmd.size()+10;
		
	RedisReplyInfo replyInfo;
	bool success=cluster->doRedisCommand(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
	{
		LOG(ERROR)<<"do info replication command failed";
		return false;
	}

	if(!ParseInfoReplicationReply(replyInfo, slaves))
	{
		freeReplyInfo(replyInfo);
		return false;
	}
	else
	{
		freeReplyInfo(replyInfo);
		return true;
	}	
}

bool RedisClient::ParseInfoReplicationReply(const RedisReplyInfo& replyInfo, vector<RedisServerInfo>& slaves)
{
	LOG(INFO)<<"info replication has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
	{
		LOG(INFO)<<"get empty list or set.";
		return false;
	}

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
	{
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}

	if(replyInfo.arrayList.empty())
	{
		LOG(ERROR)<<"recv empty arraylist";
		return false;
	}
	list<ReplyArrayInfo>::const_iterator iter = replyInfo.arrayList.begin();
	if ((*iter).replyType != RedisReplyType::REDIS_REPLY_STRING || (*iter).arrayLen == 0)
	{
		LOG(ERROR)<<"arraylist is wrong, replyType:["<<(*iter).replyType<<"], arrayLen:["<<(*iter).arrayLen<<"].";
		return false;
	}

//	const string& replicationMsg=replyInfo.resultString;
	const string& replicationMsg=iter->arrayValue;
	LOG(INFO)<<"info replication reply msg: ["<<replicationMsg<<"]";
	string::size_type startpos=0;
	string::size_type endpos=replicationMsg.find("\n");
	size_t slavescount=0;
	const string connected_slaves="connected_slaves:";
	const string slave="slave";
	const string role="role:";
	while(endpos!=string::npos)
	{
		string linemsg=replicationMsg.substr(startpos, endpos-startpos);
		if(linemsg=="\r")
			break;
//		LOG(INFO)<<"linemsg is ["<<linemsg<<"]";

		if(memcmp(linemsg.c_str(), connected_slaves.c_str(), connected_slaves.size())==0)
		{
			// [connected_slaves:2^M]
			string::size_type pos=linemsg.find('\r');
			string str=linemsg.substr(connected_slaves.size(), pos-connected_slaves.size());
			slavescount=atoi(str.c_str());
			LOG(INFO)<<"get connected_slavs value : ["<<str<<"], "<<slavescount;
		}
		else if(memcmp(linemsg.c_str(), slave.c_str(), slave.size())==0)
		{
			RedisServerInfo slaveaddr;
			// [slave0:ip=192.168.12.59,port=7101,state=online,offset=85978535,lag=0^M]
			do{
				string::size_type pos1=linemsg.find("ip=", 0);
				if(pos1==string::npos)
					break;
				string::size_type pos2=linemsg.find(',', pos1+3);
				if(pos2==string::npos)
					break;
				slaveaddr.serverIp=linemsg.substr(pos1+3, pos2-(pos1+3));
				
				pos1=linemsg.find("port=", pos2);
				if(pos1==string::npos)
					break;
				pos2=linemsg.find(',', pos1+5);
				if(pos2==string::npos)
					break;
				slaveaddr.serverPort=atoi(linemsg.substr(pos1+5, pos2-(pos1+5)).c_str());

				LOG(INFO)<<"get one slave addr: "<<slaveaddr.serverIp<<", "<<slaveaddr.serverPort;
				slaves.push_back(slaveaddr);
			}while(0);
			if(slaves.size()>=slavescount)
				break;
		}
		else if(memcmp(linemsg.c_str(), role.c_str(), role.size())==0)
		{
			// role:master
			string::size_type pos=linemsg.find('\r');
			string str=linemsg.substr(role.size(), pos-role.size());
			if(str=="master")
			{
				LOG(INFO)<<"validate role is master ok";
			}
			else
			{
				// TODO
				LOG(ERROR)<<"role is "<<str;
			}
		}
		
		startpos = endpos + 1;
		endpos = replicationMsg.find("\n", startpos);
	}

	return true;
}

bool RedisClient::AuthPasswd(const string& passwd, RedisCluster* cluster)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("auth", 4, paraList);
	paraLen += 15;
	fillCommandPara(passwd.c_str(), passwd.length(), paraList);
	paraLen += passwd.length() + 20;

	RedisReplyInfo replyInfo;
	bool success = cluster->doRedisCommand(paraList, paraLen, replyInfo);
	freeCommandList(paraList);
	if(!success)
		return false;
	success=ParseAuthReply(replyInfo);	
	freeReplyInfo(replyInfo);
	if(success)
		LOG(INFO)<<"auth passwd "<<m_passwd<<" success";
	return success;
}

bool RedisClient::ParseAuthReply(const RedisReplyInfo& replyInfo)
{
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STATUS)
	{
		LOG(ERROR)<<"auth passwd failed, redis response:["<<replyInfo.resultString<<"].";
		return false;
	}
	return true;
}

bool RedisClient::getSerial(const string & key,DBSerialize & serial)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("get", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	bool success = false;
	success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_GET, &serial);
	freeCommandList(paraList);
	return success;
}

bool RedisClient::getSerialWithLock(const string & key,DBSerialize & serial,RedisLockInfo & lockInfo)
{
	//add for  redis Optimistic Lock command, includes watch key, get key, or unwatch,when get key failed.
	bool success = false;
	list<RedisCmdParaInfo> watchParaList;
	int32_t watchParaLen = 0;
	fillCommandPara("watch", 5, watchParaList);
	watchParaLen += 18;
	fillCommandPara(key.c_str(), key.length(), watchParaList);
	watchParaLen += key.length() + 20;
	success = doRedisCommandWithLock(key,watchParaLen,watchParaList,RedisCommandType::REDIS_COMMAND_WATCH,lockInfo);
	freeCommandList(watchParaList);
	if (!success)
	{
//		m_logger.warn("do watch key:%s failed.", key.c_str());
		LOG(ERROR)<<"do watch key:"<<key<<" failed.";
		return false;
	}
	list<RedisCmdParaInfo> getParaList;
	int32_t getParaLen = 0;
	fillCommandPara("get", 3, getParaList);
	getParaLen += 15;
	fillCommandPara(key.c_str(), key.length(), getParaList);
	getParaLen += key.length() + 20;
	success = doRedisCommandWithLock(key,getParaLen,getParaList,RedisCommandType::REDIS_COMMAND_GET,lockInfo,true,&serial);
	freeCommandList(getParaList);
	if (success)
	{
//		m_logger.info("get key:%s serial with optimistic lock success.", key.c_str());
		LOG(INFO)<<"get key:"<<key<<" serial with optimistic lock success.";
	}
	else
	{
//		m_logger.warn("get key:%s serial with optimistic lock failed.", key.c_str());
		LOG(ERROR)<<"get key:"<<key<<" serial with optimistic lock failed.";
		list<RedisCmdParaInfo> unwatchParaList;
		int32_t unwatchParaLen = 0;
		fillCommandPara("unwatch", 7, unwatchParaList);
		unwatchParaLen += 20;
		doRedisCommandWithLock(key,unwatchParaLen,unwatchParaList,RedisCommandType::REDIS_COMMAND_UNWATCH,lockInfo);
		freeCommandList(unwatchParaList);
	}
	return success;
}

bool RedisClient::setSerialWithLock(const string & key, const DBSerialize& serial,RedisLockInfo & lockInfo)
{
	//add for  redis Optimistic Lock command, includes multi, set key, exec.
	bool success = false;
	//do multi command.
	list<RedisCmdParaInfo> multiParaList;
	int32_t multiParaLen = 0;
	fillCommandPara("multi", 5, multiParaList);
	multiParaLen += 18;
	success = doRedisCommandWithLock(key,multiParaLen,multiParaList,RedisCommandType::REDIS_COMMAND_MULTI,lockInfo);
	freeCommandList(multiParaList);
	if (!success)
	{
//		m_logger.warn("do multi key:%s failed.", key.c_str());
		LOG(ERROR)<<"do multi key:"<<key<<" failed.";
		list<RedisCmdParaInfo> unwatchParaList;
		int32_t unwatchParaLen = 0;
		fillCommandPara("unwatch", 7, unwatchParaList);
		unwatchParaLen += 20;
		doRedisCommandWithLock(key,unwatchParaLen,unwatchParaList,RedisCommandType::REDIS_COMMAND_UNWATCH,lockInfo);
		freeCommandList(unwatchParaList);
		return false;
	}
	//do set key value
	list<RedisCmdParaInfo> setParaList;
	int32_t setParaLen = 0;
	fillCommandPara("set", 3, setParaList);
	setParaLen += 15;
	fillCommandPara(key.c_str(), key.length(), setParaList);
	setParaLen += key.length() + 20;
	DBOutStream out;
	serial.save(out);
	fillCommandPara(out.getData(), out.getSize(), setParaList);
	setParaLen += out.getSize() + 20;
	success = doRedisCommandWithLock(key,setParaLen,setParaList,RedisCommandType::REDIS_COMMAND_SET,lockInfo);
	freeCommandList(setParaList);
	if (success)
	{
//		m_logger.info("set key:%s serial with optimistic lock success.", key.c_str());
		LOG(INFO)<<"set key:"<<key<<" serial with optimistic lock success.";
	}
	else
	{
//		m_logger.warn("set key:%s serial with optimistic lock failed.", key.c_str());
		LOG(ERROR)<<"set key:"<<key<<" serial with optimistic lock failed.";
		list<RedisCmdParaInfo> unwatchParaList;
		int32_t unwatchParaLen = 0;
		fillCommandPara("unwatch", 7, unwatchParaList);
		unwatchParaLen += 20;
		doRedisCommandWithLock(key,unwatchParaLen,unwatchParaList,RedisCommandType::REDIS_COMMAND_UNWATCH,lockInfo);
		freeCommandList(unwatchParaList);
		return false;
	}
	//do exec,need check exec response.
	list<RedisCmdParaInfo> execParaList;
	int32_t execParaLen = 0;
	fillCommandPara("exec", 4, execParaList);
	execParaLen += 18;
	success = doRedisCommandWithLock(key,execParaLen,execParaList,RedisCommandType::REDIS_COMMAND_EXEC,lockInfo);
	freeCommandList(execParaList);
	if (!success)
	{
//		m_logger.warn("update data error,key[%s]",key.c_str());
		LOG(ERROR)<<"update data error,key["<<key<<"]";
        return false;
	}
	return success;
}

bool RedisClient::releaseLock(const string & key,RedisLockInfo & lockInfo)
{
	if (lockInfo.clusterId.empty())
	{
//		m_logger.warn("lock cluster id is empty.");
		LOG(ERROR)<<"lock cluster id is empty.";
		return false;
	}
	list<RedisCmdParaInfo> unwatchParaList;
	int32_t unwatchParaLen = 0;
	fillCommandPara("unwatch", 7, unwatchParaList);
	unwatchParaLen += 20;
	doRedisCommandWithLock(key,unwatchParaLen,unwatchParaList,RedisCommandType::REDIS_COMMAND_UNWATCH,lockInfo);
	freeCommandList(unwatchParaList);
	return true;
}

bool RedisClient::find(const string& key)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("exists", 6, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	bool success = false;
	success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_EXISTS);
	freeCommandList(paraList);
	return success;
}

bool RedisClient::delKeys(const string & keyPrefix)
{
	list<string> keys;
	getKeys(keyPrefix,keys);

	list<string>::iterator it;
	for(it = keys.begin();it != keys.end();it++)
	{
		del(*it);
	}
	keys.clear();
	return true;
}

bool RedisClient::setSerial(const string& key, const DBSerialize& serial)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("set", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	DBOutStream out;
	serial.save(out);
	fillCommandPara(out.getData(), out.getSize(), paraList);
	paraLen += out.getSize() + 20;
	bool success = false;
	success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_SET);
	freeCommandList(paraList);
	return success;
}

bool RedisClient::setSerial(const string& key, const string& value)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("set", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;	
	fillCommandPara(value.c_str(), value.size(), paraList);
	paraLen += value.size() + 20;
	bool success = false;
	success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_SET);
	freeCommandList(paraList);
	return success;
}

bool RedisClient::setSerialWithExpire(const string & key, const DBSerialize & serial,uint32_t expireTime)
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
	serial.save(out);
	fillCommandPara(out.getData(), out.getSize(), paraList);
	paraLen += out.getSize() + 20;
	bool success = false;
	success = doRedisCommand(key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_SET);
	freeCommandList(paraList);
	return success;
}

bool RedisClient::del(const string & key)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("del", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	bool success = false;
	success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_DEL);
	freeCommandList(paraList);
	return success;
}

bool RedisClient::doRedisCommand(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType)
{
	list<string> members;
	return doRedisCommand(key, commandLen, commandList, commandType, members, NULL, NULL);
}

bool RedisClient::doRedisCommand(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType,
							DBSerialize* serial)
{
	list<string> members;
	return doRedisCommand(key, commandLen, commandList, commandType, members, serial, NULL);
}

bool RedisClient::doRedisCommand(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType,
							int* count)
{
	list<string> members;
	return doRedisCommand(key, commandLen, commandList, commandType, members, NULL, count);
}

bool RedisClient::doRedisCommand(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType,
							list<string>& members)
{
	return doRedisCommand(key, commandLen, commandList, commandType, members, NULL, NULL);
}


bool RedisClient::doRedisCommand(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType, 
							list<string>& members,
							DBSerialize* serial, int* count)
{
	if(m_redisMode==CLUSTER_MODE)
	{
		return doRedisCommandCluster(key, commandLen, commandList, commandType, members, serial, count);
	}
	else if(m_redisMode==STAND_ALONE_OR_PROXY_MODE)
	{
		return doRedisCommandStandAlone(key, commandLen, commandList, commandType, members, serial, count);
	}
	else if(m_redisMode==SENTINEL_MODE)
	{
		return doRedisCommandMaster(key, commandLen, commandList, commandType, members, serial, count);
	}
	return false;
}

bool RedisClient::doRedisCommandStandAlone(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType,
							list<string>& members,
							DBSerialize* serial, int* count)
{
	RedisReplyInfo replyInfo;
	bool needRedirect;
	string redirectInfo;
    if(m_redisProxy.clusterHandler==NULL)
    {
		LOG(ERROR)<<"m_redisProxy.clusterHandler is NULL";
        return false;
    }
	if(!m_redisProxy.clusterHandler->doRedisCommand(commandList, commandLen, replyInfo))
	{
		freeReplyInfo(replyInfo);
		LOG(ERROR)<<"proxy: "<<m_redisProxy.proxyId<<" do redis command failed.";
		return false;
	}

	bool success=ParseRedisReplyForStandAloneAndMasterMode(replyInfo, needRedirect, redirectInfo, key, commandType, members, serial, count);
	freeReplyInfo(replyInfo);
	return success;
}

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
			if(!parseGetSerialReply(replyInfo,*serial,needRedirect,redirectInfo))
			{
				LOG(ERROR)<<"parse key:["<<key<<"] get string reply failed. reply string:"<<replyInfo.resultString<<".";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_SET:
		case RedisCommandType::REDIS_COMMAND_AUTH:
			if(!parseSetSerialReply(replyInfo,needRedirect,redirectInfo))
			{
				LOG(ERROR)<<"parse key:["<<key<<"] get string reply failed. reply string:"<<replyInfo.resultString<<".";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_EXISTS:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
				LOG(ERROR)<<"parse key:["<<key<<"] get string reply failed. reply string:"<<replyInfo.resultString<<".";
				freeReplyInfo(replyInfo);
				return false;
			}
			//find
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
				LOG(ERROR)<<"find key:"<<key<<" in redis db";
				freeReplyInfo(replyInfo);
				return true;
			}
			//not find
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
				LOG(ERROR)<<"not find key:"<<key<<" in redis db";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_DEL:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
				LOG(ERROR)<<"parse key:["<<key<<"] del string reply failed. reply string:"<<replyInfo.resultString<<".";
				freeReplyInfo(replyInfo);
				return false;
			}
			//del success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
				LOG(INFO)<<"del key:"<<key<<" from redis db success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			//del failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//				m_logger.warn("del key:%s from redis db failed.",key.c_str());
				LOG(WARNING)<<"del key:"<<key<<" from redis db failed.";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_EXPIRE:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//				m_logger.warn("parse key:[%s] set expire reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] set expire reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			//set expire success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//				m_logger.debug("set key:%s expire success.",key.c_str());
				LOG(INFO)<<"set key:"<<key<<" expire success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			//set expire failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//				m_logger.warn("set key:%s expire failed.",key.c_str());
				LOG(ERROR)<<"set key:"<<key<<" expire failed.";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_ZADD:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//				m_logger.warn("parse key:[%s] zset add reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] zset add reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			// success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//				m_logger.debug("zset key:%s add success.",key.c_str());
				LOG(INFO)<<"zset key:"<<key<<" add success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//				m_logger.debug("zset key:%s add done,maybe exists.",key.c_str());
				LOG(WARNING)<<"zset key:"<<key<<" add done,maybe exists.";
				freeReplyInfo(replyInfo);
				return false;
			}				
			break;
		case RedisCommandType::REDIS_COMMAND_ZREM:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//				m_logger.warn("parse key:[%s] zset rem reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] zset rem reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			// success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//				m_logger.debug("set key:%s rem success.",key.c_str());
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//				m_logger.warn("set key:%s rem failed.",key.c_str());
				LOG(ERROR)<<"set key:"<<key<<" rem failed.";
				freeReplyInfo(replyInfo);
				return false;
			}

			break;
		case RedisCommandType::REDIS_COMMAND_ZINCRBY:
			// success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_STRING)
			{
//				m_logger.debug("set key:%s zincrby success.",key.c_str());
				LOG(ERROR)<<"set key:"<<key<<" zincrby success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else
			{
//				m_logger.warn("set key:%s zincrby failed.",key.c_str());
				LOG(ERROR)<<"set key: "<<key<<"zincrby failed.";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;

		case RedisCommandType::REDIS_COMMAND_ZREMRANGEBYSCORE:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//				m_logger.warn("parse key:[%s] zset zremrangebyscore reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] zset zremrangebyscore reply failed. reply string:"<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			// success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue > 0)
			{
//				m_logger.debug("set key:%s zremrangebyscore success.",key.c_str());
				LOG(INFO)<<"set key:"<<key<<" zremrangebyscore success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//				m_logger.warn("set key:%s zremrangebyscore failed.",key.c_str());
				LOG(ERROR)<<"set key:"<<key<<" zremrangebyscore failed.";
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
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//				m_logger.warn("parse key:[%s] zset add reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] zset add reply failed. reply string:"<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER)
			{
				if(count)
					*count = replyInfo.intValue;
//				m_logger.info("key %s commandType %d success, return integer %d", key.c_str(), commandType, replyInfo.intValue);
				LOG(INFO)<<"key "<<key<<" commandType "<<commandType<<" success, return integer "<<replyInfo.intValue;
				freeReplyInfo(replyInfo);
				return true;
			}
			else
			{
//				m_logger.warn("key %s commandType %d, return error", key.c_str(), commandType);
				LOG(ERROR)<<"key "<<key<<" commandType "<<commandType<<", return error";
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_ZSCORE:			
			{	
				if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
				{
//					m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
					LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"]";
					freeReplyInfo(replyInfo);
					return false;
				}
				list<ReplyArrayInfo>::iterator iter = replyInfo.arrayList.begin();
				if (iter == replyInfo.arrayList.end())
				{
//					m_logger.warn("reply not have array info.");
					LOG(ERROR)<<"reply not have array info.";
					freeReplyInfo(replyInfo);
					return false;
				}
				if ((*iter).replyType == RedisReplyType::REDIS_REPLY_NIL)
				{
//					m_logger.warn("get failed,the key not exist.");
					LOG(ERROR)<<"get failed,the key not exist.";
					freeReplyInfo(replyInfo);
					return false;
				}
				char score_c[64] = {0};
				memcpy(score_c,(*iter).arrayValue,(*iter).arrayLen);
				if(count==NULL)
					return false;
				*count = atoi(score_c);
//				m_logger.info("key:%s,score:%d",key.c_str(), *count);
				LOG(INFO)<<"key:"<<key<<", score:"<<*count;
				return true;	
			}
			break;
		case RedisCommandType::REDIS_COMMAND_ZRANGEBYSCORE:
		case RedisCommandType::REDIS_COMMAND_SMEMBERS:
			if(parseGetKeysReply(replyInfo, members)==false)
				return false;
			break;		
		default:
			LOG(ERROR)<<"recv unknown command type: "<<commandType;
			return false;
	}
	return true;
}

//bool RedisClient::CreateConnectionPool(map<string, RedisProxyInfo>::iterator& it)
//{
//	if(it->second.clusterHandler==NULL)
//	{
//		it->second.clusterHandler=new RedisCluster();
//		if(it->second.clusterHandler==NULL)
//			return false;
//	}
//
//	if(!it->second.clusterHandler->initConnectPool(it->second.connectIp, it->second.connectPort, m_connectionNum, m_connectTimeout, m_readTimeout))
//	{
//		LOG(ERROR)<<"init connection pool failed for "<<it->second.proxyId;
//		return false;
//	}
//	it->second.isAlived=true;
//	return true;
//}

bool RedisClient::CreateConnectionPool(RedisProxyInfo& node)
{
	if(node.clusterHandler==NULL)
	{
		node.clusterHandler=new RedisCluster();
		if(node.clusterHandler==NULL)
			return false;
	}

	if(!node.clusterHandler->initConnectPool(node.connectIp, node.connectPort, m_connectionNum, m_connectTimeout, m_readTimeout))
	{
		LOG(ERROR)<<"init connection pool failed for "<<node.proxyId;
		return false;
	}
	node.isAlived=true;
	return true;
}

bool RedisClient::doRedisCommandMaster(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType,
							list<string>& members,
							DBSerialize * serial, int* count)
{
	RedisReplyInfo replyInfo;
	bool needRedirect;
	string redirectInfo;

	RedisProxyInfo &masterNode=m_dataNodes[m_masterClusterId];
	if(masterNode.clusterHandler==NULL  ||  masterNode.isAlived==false)
	{
		LOG(ERROR)<<"master node "<<m_masterClusterId<<" not alive";
//		CreateConnectionPool(masterNode);
        return false;
    }
	if(!masterNode.clusterHandler->doRedisCommand(commandList, commandLen, replyInfo))
	{
		freeReplyInfo(replyInfo);
		LOG(ERROR)<<"master: "<<masterNode.proxyId<<" do redis command failed.";

		if(IsCommandWriteType(commandType))
		{
			LOG(WARNING)<<"commandType is "<<commandType<<", cannot send to slave";
			return false;
		}

		bool success=false;
		for(map<string, RedisProxyInfo>::iterator it=m_dataNodes.begin(); it!=m_dataNodes.end(); ++it)
		{
			if(it->second.proxyId==m_masterClusterId)
				continue;
//			if(it->second.isAlived==false)
//				CreateConnectionPool(it);
			if(it->second.isAlived  &&  it->second.clusterHandler->doRedisCommand(commandList, commandLen, replyInfo))
			{
				LOG(INFO)<<"succeed to send to slave "<<it->second.proxyId;
				success=true;
				break;
			}
			else
			{
				freeReplyInfo(replyInfo);
			}
		}
		if(!success)
		{
			freeReplyInfo(replyInfo);
			LOG(ERROR)<<"try slaves failed";
			return false;
		}
	}

	
	bool success=ParseRedisReplyForStandAloneAndMasterMode(replyInfo, needRedirect, redirectInfo, key, commandType, members, serial, count);
	freeReplyInfo(replyInfo);
	return success;
}


bool RedisClient::doRedisCommandCluster(const string & key,
							int32_t commandLen,
							list < RedisCmdParaInfo > & commandList,
							RedisCommandType commandType,
							list<string>& members,
							DBSerialize * serial, int* count)
{
	assert(m_redisMode==CLUSTER_MODE);

	uint16_t crcValue = crc16(key.c_str(), key.length());
	crcValue %= REDIS_SLOT_NUM;

	string clusterId;	
	if (getClusterIdBySlot(crcValue, clusterId)==false)
	{
		LOG(ERROR)<<"key:["<<key<<"] hit slot:["<<crcValue<<"] select cluster failed.";
		return false;
	}
	LOG(INFO)<<"key:["<<key<<"] hit slot:["<<crcValue<<"] select cluster["<<clusterId<<"].";

	//add for redirect end endless loop;
	vector<string> redirects;
	RedisReplyInfo replyInfo;
	list<string> bakClusterList;
	list<string>::iterator bakIter;

REDIS_COMMAND:
	RedisClusterInfo clusterInfo;
	if (getRedisClusterInfo(clusterId,clusterInfo)==false)
	{
		LOG(ERROR)<<"key:["<<key<<"] hit slot:["<<crcValue<<"], but not find cluster:["<<clusterId<<"].";
		return false;
	}
	
	if(!clusterInfo.clusterHandler->doRedisCommand(commandList, commandLen, replyInfo))
	{
		freeReplyInfo(replyInfo);
		LOG(ERROR)<<"cluster:"<<clusterId<<" do redis command failed.";
		
		if(IsCommandWriteType(commandType))
		{
			LOG(WARNING)<<"commandType is "<<commandType<<", cannot send to slave";
			return false;
		}
		
		//need send to another cluster. check bak cluster.
        if(bakClusterList.empty()==false)
        {
            bakIter++;
            if (bakIter != bakClusterList.end())
            {
                clusterId = (*bakIter);
                goto REDIS_COMMAND;
            }
            else
            {
				LOG(ERROR)<<"key:["<<key<<"] send to all bak cluster failed";
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
				LOG(ERROR)<<"key:["<<key<<"] send to all bak cluster failed";
                return false;
            }
        }
//				if (clusterInfo.bakClusterList.size() != 0)
//				{
//					bakClusterList = clusterInfo.bakClusterList;
//					bakIter = bakClusterList.begin();
//					if (bakIter != bakClusterList.end())
//					{
//						clusterId = (*bakIter);
//						goto REDIS_COMMAND;
//					}
//					else
//					{
//						m_logger.warn("key:[%s] send to all redis cluster failed, may be slot:[%d] is something wrong", key.c_str(), crcValue);
//						return false;
//					}
//				}
//				else
//				{
//					if (bakClusterList.size() > 0)
//					{
//						bakIter++;
//						if (bakIter != bakClusterList.end())
//						{
//							clusterId = (*bakIter);
//							goto REDIS_COMMAND;
//						}
//						else
//						{
//							m_logger.warn("key:[%s] send to all redis cluster failed, may be slot:[%d] is something wrong", key.c_str(), crcValue);
//							return false;
//						}
//					}
//				}
	}
	
	bool needRedirect = false;
	string redirectInfo;

	LOG(INFO)<<"start to parse command type:"<<commandType<<" redis reply.";

	switch (commandType)
	{
		case RedisCommandType::REDIS_COMMAND_GET:					
			if(!parseGetSerialReply(replyInfo,*serial,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] get string reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] get string reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_SET:
		case RedisCommandType::REDIS_COMMAND_AUTH:
			if(!parseSetSerialReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] set string reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] get string reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_EXISTS:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] get string reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] get string reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			//find
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//						m_logger.debug("find key:%s in redis db",key.c_str());
				LOG(INFO)<<"find key:"<<key<<" in redis db";
				freeReplyInfo(replyInfo);
				return true;
			}
			//not find
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//						m_logger.warn("not find key:%s in redis db",key.c_str());
				LOG(ERROR)<<"not find key:"<<key<<" in redis db";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_DEL:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] del string reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] del string reply failed. reply string:"<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			//del success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//						m_logger.debug("del key:%s from redis db success.",key.c_str());
				LOG(INFO)<<"del key:"<<key<<" from redis db success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			//del failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//						m_logger.warn("del key:%s from redis db failed.",key.c_str());
				LOG(WARNING)<<"del key:"<<key<<" from redis db failed.";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_EXPIRE:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] set expire reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] set expire reply failed. reply string:"<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			//set expire success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//						m_logger.debug("set key:%s expire success.",key.c_str());
				LOG(INFO)<<"set key:"<<key<<" expire success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			//set expire failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//						m_logger.warn("set key:%s expire failed.",key.c_str());
				LOG(ERROR)<<"set key:"<<key<<" expire failed.";
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_ZADD:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] zset add reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] zset add reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			// success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//						m_logger.debug("zset key:%s add success.",key.c_str());
				LOG(INFO)<<"zset key:"<<key<<" add success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//						m_logger.debug("zset key:%s add done,maybe exists.",key.c_str());
				LOG(INFO)<<"zset key:"<<key<<" add done,maybe exists.";
				freeReplyInfo(replyInfo);
				return false;
			}				
			break;
		case RedisCommandType::REDIS_COMMAND_ZREM:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] zset rem reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] zset rem reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			// success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 1)
			{
//						m_logger.debug("set key:%s rem success.",key.c_str());
				LOG(INFO)<<"set key:"<<key<<" rem success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//						m_logger.warn("set key:%s rem failed.",key.c_str());
				LOG(ERROR)<<"set key:"<<key<<" rem failed.";
				freeReplyInfo(replyInfo);
				return false;
			}

			break;
		case RedisCommandType::REDIS_COMMAND_ZINCRBY:
			if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
			{
//						m_logger.info("need direct to cluster:[%s].", redirectInfo.c_str());
				LOG(INFO)<<"need direct to cluster:["<<redirectInfo<<"]";
			}
			else
			{
				// success
				if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_STRING)
				{
//							m_logger.debug("set key:%s zincrby success.",key.c_str());
					LOG(INFO)<<"set key:"<<key<<" zincrby success.";
					freeReplyInfo(replyInfo);
					return true;
				}
				// failed
				else
				{
//							m_logger.warn("set key:%s zincrby failed.",key.c_str());
					LOG(ERROR)<<"set key:"<<key<<" zincrby failed.";
					freeReplyInfo(replyInfo);
					return false;
				}
			}
			break;

		case RedisCommandType::REDIS_COMMAND_ZREMRANGEBYSCORE:
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] zset zremrangebyscore reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] zset zremrangebyscore reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			// success
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue > 0)
			{
//						m_logger.debug("set key:%s zremrangebyscore success.",key.c_str());
				LOG(INFO)<<"set key:"<<key<<" zremrangebyscore success.";
				freeReplyInfo(replyInfo);
				return true;
			}
			// failed
			else if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER && replyInfo.intValue == 0)
			{
//						m_logger.warn("set key:%s zremrangebyscore failed.",key.c_str());
				LOG(ERROR)<<"set key:"<<key<<" zremrangebyscore failed.";
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
			if(!parseFindReply(replyInfo,needRedirect,redirectInfo))
			{
//						m_logger.warn("parse key:[%s] zset add reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse key:["<<key<<"] get reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			
			if(replyInfo.replyType == RedisReplyType::REDIS_REPLY_INTEGER)
			{
				if(count)
					*count = replyInfo.intValue;
//						m_logger.info("key %s commandType %d success, return integer %d", key.c_str(), commandType, replyInfo.intValue);
				LOG(INFO)<<"key "<<key<<" commandType "<<commandType<<" success, return integer "<<replyInfo.intValue;
				freeReplyInfo(replyInfo);
				return true;
			}
			break;
		case RedisCommandType::REDIS_COMMAND_ZSCORE:
			if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
			{
//						m_logger.info("need direct to cluster:[%s].", redirectInfo.c_str());
				LOG(WARNING)<<"need direct to cluster:["<<redirectInfo<<"]";
			}
			else
			{	
				if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
				{
//							m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
					LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
					freeReplyInfo(replyInfo);
					return false;
				}
				list<ReplyArrayInfo>::iterator iter = replyInfo.arrayList.begin();
				if (iter == replyInfo.arrayList.end())
				{
//							m_logger.warn("reply not have array info.");
					LOG(WARNING)<<"reply not have array info.";
					freeReplyInfo(replyInfo);
					return false;
				}
				if ((*iter).replyType == RedisReplyType::REDIS_REPLY_NIL)
				{
//							m_logger.warn("get failed,the key not exist.");
					LOG(WARNING)<<"get failed,the key not exist.";
					freeReplyInfo(replyInfo);
					return false;
				}
				char score_c[64] = {0};
				memcpy(score_c,(*iter).arrayValue,(*iter).arrayLen);
				if(count)
					*count = atoi(score_c);
//						m_logger.info("key:%s,score:%d",key.c_str(), atoi(score_c));
				LOG(INFO)<<"key:"<<key<<", score:"<<atoi(score_c);
				return true;	
			}
			break;
		case RedisCommandType::REDIS_COMMAND_ZRANGEBYSCORE:
		case RedisCommandType::REDIS_COMMAND_SMEMBERS:
			if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
			{
				LOG(INFO)<<"need direct to cluster:["<<redirectInfo<<"].";
			}
			else
			{
				parseGetKeysReply(replyInfo, members);
			}
			break;
		default:
			LOG(ERROR)<<"recv unknown command type:"<<commandType;
			break;
	}
	
	freeReplyInfo(replyInfo);
	if (needRedirect)
	{
		SignalToDoClusterNodes();
		
		LOG(INFO)<<"key:["<<key<<"] need redirect to cluster:["<<redirectInfo<<"].";

		//check cluster redirect if exist.
		vector<string>::iterator reIter;
		reIter = ::find(redirects.begin(), redirects.end(), redirectInfo);
		if (reIter == redirects.end())
		{
			redirects.push_back(redirectInfo);
			
			clusterId = redirectInfo;
			bakClusterList.clear();
			goto REDIS_COMMAND;
//            if(getClusterIdFromRedirectReply(redirectInfo, clusterId))
//            {
//                bakClusterList.clear();
//				goto REDIS_COMMAND;
//            }
//            else
//            {
//				LOG(ERROR)<<"no cluster of redirect info "<<redirectInfo;
//                return false;
//            }
		}
		else
		{
			LOG(ERROR)<<"redirect:"<<redirectInfo<<" is already do redis command,the slot:["<<crcValue<<"] may be removed by redis.please check it.";
			return false;
		}
	}
	
	return true;
}

bool RedisClient::StartCheckClusterThread()
{
	if(m_checkClusterNodesThreadStarted)
		return true;
	int ret=pthread_create(&m_checkClusterNodesThreadId, NULL, CheckClusterNodesThreadFunc, this);
	if(ret)
	{
		PLOG(ERROR)<<"start check cluster nodes thread failed";
		return false;
	}
	m_checkClusterNodesThreadStarted=true;
	return true;
}

void RedisClient::SignalToDoClusterNodes()
{
	MutexLockGuard guard(&m_lockSignalQueue);
	if(m_signalQueue.empty())
	{
		m_signalQueue.push(1);
	}
	m_condSignalQueue.SignalAll();
}

void* RedisClient::CheckClusterNodesThreadFunc(void* arg)
{
	LOG(INFO)<<"check cluster nodes thread start";

	RedisClient* client=(RedisClient*)arg;

	while(true)
	{
		{
			MutexLockGuard guard(&(client->m_lockSignalQueue));
			while(client->m_signalQueue.empty())
			{
				client->m_condSignalQueue.Wait();
			}
		}

		client->m_signalQueue.swap(queue<int>());		
		LOG(INFO)<<"now recv signal to do cluster nodes command";

		client->DoCheckClusterNodes();
	}
	return 0;
}

void RedisClient::DoCheckClusterNodes()
{
	releaseUnusedClusterHandler();

	REDIS_CLUSTER_MAP clusterMap;
	if (!getRedisClustersByCommand(clusterMap))
	{
		LOG(ERROR)<<"get redis clusters by command failed.";
		return;
	}
	REDIS_CLUSTER_MAP oldClusterMap;
	getRedisClusters(oldClusterMap);
	//check cluster if change.
	bool change = false;
	REDIS_CLUSTER_MAP::iterator clusterIter;
	for (clusterIter = clusterMap.begin(); clusterIter != clusterMap.end(); clusterIter++)
	{
		string clusterId = (*clusterIter).first;
		//check clusterId if exist.
		if (oldClusterMap.find(clusterId) == oldClusterMap.end())
		{
			change = true;
			break;
		}
		else
		{
			//check slot or master,alive status if change.
			RedisClusterInfo clusterInfo = (*clusterIter).second;
			RedisClusterInfo oldClusterInfo = oldClusterMap[clusterId];
			if (clusterInfo.clusterId != oldClusterInfo.clusterId 
				|| clusterInfo.isMaster != oldClusterInfo.isMaster 
				|| clusterInfo.isAlived != oldClusterInfo.isAlived 
				|| clusterInfo.masterClusterId != oldClusterInfo.masterClusterId
				|| clusterInfo.scanCursor != oldClusterInfo.scanCursor)
			{
				change = true;
				break;
			}
			//check bak cluster list if change.first check new cluster info
			list<string>::iterator bakIter;
			for (bakIter = clusterInfo.bakClusterList.begin(); bakIter != clusterInfo.bakClusterList.end(); bakIter++)
			{
				bool find = false;
				list<string>::iterator oldBakIter;
				for (oldBakIter = oldClusterInfo.bakClusterList.begin(); oldBakIter != oldClusterInfo.bakClusterList.end(); oldBakIter++)
				{
					if ((*bakIter) == (*oldBakIter))
					{
						find = true;
						break;
					}
				}
				if (!find)
				{
					change = true;
					break;
				}
			}
			if (change)
			{
				break;
			}
			//second check old cluster info.
			for (bakIter = oldClusterInfo.bakClusterList.begin(); bakIter != oldClusterInfo.bakClusterList.end(); bakIter++)
			{
				bool find = false;
				list<string>::iterator iter;
				for (iter = clusterInfo.bakClusterList.begin(); iter != clusterInfo.bakClusterList.end(); iter++)
				{
					if ((*bakIter) == (*iter))
					{
						find = true;
						break;
					}
				}
				if (!find)
				{
					change = true;
					break;
				}
			}
			if (change)
			{
				break;
			}
			//check slot map,first check new cluster slot map
			map<uint16_t,uint16_t>::iterator slotIter;
			for (slotIter = clusterInfo.slotMap.begin(); slotIter != clusterInfo.slotMap.end(); slotIter++)
			{
				uint16_t startSlotNum = (*slotIter).first;
				if (oldClusterInfo.slotMap.find(startSlotNum) == oldClusterInfo.slotMap.end())
				{
					change = true;
					break;
				}
				else
				{
					if ((*slotIter).second != oldClusterInfo.slotMap[startSlotNum])
					{
						change = true;
						break;
					}
				}
			}
			if (change)
			{
				break;
			}
			//second check old cluster slot map.
			for (slotIter = oldClusterInfo.slotMap.begin(); slotIter != oldClusterInfo.slotMap.end(); slotIter++)
			{
				uint16_t startSlotNum = (*slotIter).first;
				if (clusterInfo.slotMap.find(startSlotNum) == clusterInfo.slotMap.end())
				{
					change = true;
					break;
				}
				else
				{
					if ((*slotIter).second != clusterInfo.slotMap[startSlotNum])
					{
						change = true;
						break;
					}
				}
			}
			if (change)
			{
				break;
			}
		}
	}
	
	if (change)
	{
		checkAndSaveRedisClusters(clusterMap);
		return;
	}
	for (clusterIter = oldClusterMap.begin(); clusterIter != oldClusterMap.end(); clusterIter++)
	{
		string clusterId = (*clusterIter).first;
		//check clusterId if exist.
		if (clusterMap.find(clusterId) == clusterMap.end())
		{
			change = true;
			break;
		}
	}
	if (change)
	{
		checkAndSaveRedisClusters(clusterMap);
		return;
	}
}

bool RedisClient::getClusterIdFromRedirectReply(const string& redirectInfo, string& clusterId)
{
    string::size_type pos=redirectInfo.find(":");
    if(pos==string::npos)
        return false;
    string redirectIp=redirectInfo.substr(0, pos);
    string redirectPort=redirectInfo.substr(pos+1);
    for(REDIS_CLUSTER_MAP::iterator it=m_clusterMap.begin(); it!=m_clusterMap.end(); it++)
    {
        if(it->second.connectIp==redirectIp  &&  (uint16_t)(atoi(redirectPort.c_str()))==(uint16_t)it->second.connectPort)
        {
            clusterId=it->second.clusterId;
            return true;
        }
    }
    return false;
}

bool RedisClient::doRedisCommandWithLock(const string & key,int32_t commandLen,list < RedisCmdParaInfo > & commandList,int commandType,RedisLockInfo & lockInfo,bool getSerial,DBSerialize * serial)
{
	assert(m_redisMode==CLUSTER_MODE);
	//first calc crc16 value.
	uint16_t crcValue = crc16(key.c_str(), key.length());
	crcValue %= REDIS_SLOT_NUM;
	string clusterId;
	if (lockInfo.clusterId.empty())
	{
		if (!getClusterIdBySlot(crcValue, clusterId))
		{
//			m_logger.warn("key:[%s] hit slot:[%d] select cluster failed.", key.c_str(), crcValue);
			LOG(ERROR)<<"key:["<<key<<"] hit slot:["<<crcValue<<"] select cluster failed.";
			return false;
		}
	}
	else
	{
		clusterId = lockInfo.clusterId;
	}
	//add for redirect end endless loop;
	bool release = false;
	if (commandType == RedisCommandType::REDIS_COMMAND_EXEC || commandType == RedisCommandType::REDIS_COMMAND_UNWATCH)
	{
		release = true;
	}
	vector<string> redirects;
REDIS_COMMAND:
	RedisReplyInfo replyInfo;
//	m_logger.debug("key:[%s] hit slot:[%d] select cluster[%s].", key.c_str(), crcValue, clusterId.c_str());
	LOG(INFO)<<"key:["<<key<<"] hit slot:["<<crcValue<<"] select cluster.";
	list<string> bakClusterList;
	bakClusterList.clear();
	list<string>::iterator bakIter;

	//get cluster
	RedisClusterInfo clusterInfo;
	if (getRedisClusterInfo(clusterId,clusterInfo))
	{
		if(!clusterInfo.clusterHandler->doRedisCommandOneConnection(commandList, commandLen, replyInfo, release, &lockInfo.connection))
		{
			freeReplyInfo(replyInfo);
//			m_logger.warn("cluster:%s do redis command failed.", clusterId.c_str());
			
			//need check 
			if (!lockInfo.clusterId.empty())
			{
//				m_logger.warn("the transaction must do in one connection.");
				LOG(ERROR)<<"the transaction must do in one connection.";
				return false;
			}
			//need send to another cluster. check bak cluster.
			if (clusterInfo.bakClusterList.size() != 0)
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
//					m_logger.warn("key:[%s] send to all redis cluster failed, may be slot:[%d] is something wrong", key.c_str(), crcValue);
					return false;
				}
			}
			else
			{
				if (bakClusterList.size() > 0)
				{
					bakIter++;
					if (bakIter != bakClusterList.end())
					{
						clusterId = (*bakIter);
						goto REDIS_COMMAND;
					}
					else
					{
//						m_logger.warn("key:[%s] send to all redis cluster failed, may be slot:[%d] is something wrong", key.c_str(), crcValue);
						return false;
					}
				}
			}
		}
		bool needRedirect = false;
		string redirectInfo;
		redirectInfo.clear();
//		m_logger.debug("start to parse command type:%d redis reply.", commandType);
		//switch command type
		switch (commandType)
		{
			case RedisCommandType::REDIS_COMMAND_GET:
				if(!parseGetSerialReply(replyInfo,*serial,needRedirect,redirectInfo))
				{
//					m_logger.warn("parse key:[%s] get string reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
					freeReplyInfo(replyInfo);
					return false;
				}
				break;
			case RedisCommandType::REDIS_COMMAND_SET:
				if(!parseSetSerialReply(replyInfo,needRedirect,redirectInfo))
				{
//					m_logger.warn("parse key:[%s] set serial reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
					freeReplyInfo(replyInfo);
					return false;
				}
				break;
			case RedisCommandType::REDIS_COMMAND_WATCH:
				if(!parseStatusResponseReply(replyInfo,needRedirect,redirectInfo))
				{
//					m_logger.warn("parse key:[%s] watch reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
					freeReplyInfo(replyInfo);
					return false;
				}
				break;
			case RedisCommandType::REDIS_COMMAND_UNWATCH:
				if(!parseStatusResponseReply(replyInfo,needRedirect,redirectInfo))
				{
//					m_logger.warn("parse key:[%s] unwatch reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
					freeReplyInfo(replyInfo);
					return false;
				}
				//check UNWATCH reply,if not OK,need free connection.
				if (replyInfo.resultString != "OK")
				{
//					m_logger.info("do unwatch command reply is not OK, need free connection.");
					clusterInfo.clusterHandler->freeConnection(lockInfo.connection);
				}
				break;
			case RedisCommandType::REDIS_COMMAND_MULTI:
				if(!parseStatusResponseReply(replyInfo,needRedirect,redirectInfo))
				{
//					m_logger.warn("parse key:[%s] multi reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
					freeReplyInfo(replyInfo);
					return false;
				}
				break;
			case RedisCommandType::REDIS_COMMAND_EXEC:
				if(!parseExecReply(replyInfo,needRedirect,redirectInfo))
				{
//					m_logger.warn("parse key:[%s] exec reply failed. reply string:%s.", key.c_str(), replyInfo.resultString.c_str());
					freeReplyInfo(replyInfo);
					return false;
				}
				break;
			default:
//				m_logger.warn("recv unknown command type:%d", commandType);
				break;
		}
			
		freeReplyInfo(replyInfo);
		if (needRedirect)
		{
//			m_logger.info("key:[%s] need redirect to cluster:[%s].", key.c_str(), redirectInfo.c_str());
			
			if (!lockInfo.clusterId.empty())
			{
//				m_logger.warn("the transaction must do in one connection.");
				return false;
			}
			clusterId = redirectInfo;
			//check cluster redirect if exist.
			vector<string>::iterator reIter;
			reIter = ::find(redirects.begin(), redirects.end(), redirectInfo);
			if (reIter == redirects.end())
			{
				redirects.push_back(redirectInfo);
				goto REDIS_COMMAND;
			}
			else
			{
//				m_logger.warn("redirect:%s is already do redis command,the slot:[%d] may be removed by redis.please check it.", redirectInfo.c_str(), crcValue);
			}
		}
	}
	else
	{
//		m_logger.warn("key:[%s] hit slot:[%d], but not find cluster:[%s].", key.c_str(), crcValue, clusterId.c_str());
		return false;
	}
	if (lockInfo.clusterId.empty())
		lockInfo.clusterId = clusterId;
	return true;
}


// redis-server> scan m_cursor MATCH m_redisDbPre+index* COUNT count
void RedisClient::fillScanCommandPara(int cursor, const string& queryKey, 
		int count, list<RedisCmdParaInfo>& paraList, int32_t& paraLen, 
		ScanMode scanMode)
{
	int int32_t_max_size = 10; // 4 294 967 296, 10 chars
	paraList.clear();
	paraLen=0;
	string str="scan";
	fillCommandPara(str.c_str(), str.size(), paraList);
	paraLen += str.size()+int32_t_max_size+1;

	char buf[20];
	if(scanMode == SCAN_NOLOOP)
		cursor = 0;
	int len=snprintf(buf, sizeof(buf)/sizeof(char), "%d", cursor);
	fillCommandPara(buf, len, paraList);
	paraLen += len+int32_t_max_size+1;

	str="MATCH";
	fillCommandPara(str.c_str(), str.size(), paraList);;
	paraLen +=str.size()+int32_t_max_size+1;

	string key=queryKey+"*";
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() +int32_t_max_size+1;

	str="COUNT";
	fillCommandPara(str.c_str(), str.size(), paraList);
	paraLen += str.size() +int32_t_max_size+1;

	len=snprintf(buf, sizeof(buf)/sizeof(char), "%d", count);
	fillCommandPara(buf, len, paraList);
	paraLen += len+int32_t_max_size+1;
}


bool RedisClient::scanKeys(const string& queryKey, uint32_t count, list<string> &keys, ScanMode scanMode)
{
	uint32_t COUNT = 10000; // TODO change according to db size or a maximum number
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen =0;
	REDIS_CLUSTER_MAP clusterMap;
	getRedisClusters(clusterMap);
	REDIS_CLUSTER_MAP::iterator clusterIter;
	int retCursor=-1;
//	m_logger.debug("clusterMap size is %d", clusterMap.size());
	LOG(INFO)<<"clusterMap size is "<<clusterMap.size();
	for (clusterIter = clusterMap.begin(); clusterIter != clusterMap.end(); /*clusterIter++*/)
	{
		string clusterId = (*clusterIter).first;
		RedisClusterInfo clusterInfo;
		if(!getRedisClusterInfo(clusterId, clusterInfo))
		{
//			m_logger.warn("get redis cluster:[%s] info failed.", clusterId.c_str());
			LOG(WARNING)<<"get redis cluster:["<<clusterId<<"] info failed.";
			continue;
		}
		
		if (clusterInfo.isMaster && clusterInfo.isAlived)
		{
//			m_logger.debug("redis %s is master, alive", clusterId.c_str());
			LOG(INFO)<<"redis "<<clusterId<<" is master, alive";
			
			RedisReplyInfo replyInfo;
			fillScanCommandPara(clusterInfo.scanCursor, queryKey, COUNT, paraList, paraLen, scanMode);
			if(!clusterInfo.clusterHandler->doRedisCommand(paraList, paraLen, replyInfo, RedisConnection::SCAN_PARSER)) 
			{
				freeReplyInfo(replyInfo);
//				m_logger.debug("%s doRedisCommand failed, send bak redis cluster", clusterId.c_str());
				LOG(WARNING)<<clusterId<<" doRedisCommand failed, send bak redis cluster";

				list<string>::iterator bakIter;
				for (bakIter = clusterInfo.bakClusterList.begin(); bakIter != clusterInfo.bakClusterList.end(); /*bakIter++*/)
				{
					RedisClusterInfo bakClusterInfo;
					if(!getRedisClusterInfo((*bakIter), bakClusterInfo))
					{
//						m_logger.warn("get bak redis cluster:[%s] info.", (*bakIter).c_str());
						LOG(WARNING)<<"get bak redis cluster:["<<*bakIter<<"] info.";
						bakIter++;
						continue;
					}
					fillScanCommandPara(bakClusterInfo.scanCursor, queryKey, COUNT, paraList, paraLen, scanMode);
					if(!bakClusterInfo.clusterHandler->doRedisCommand(paraList, paraLen, replyInfo, RedisConnection::SCAN_PARSER))
					{
						freeReplyInfo(replyInfo);
//						m_logger.warn("redis cluster %s, bak redis cluster %s doRedisCommand failed", clusterInfo.clusterId.c_str(), bakClusterInfo.clusterId.c_str());
						LOG(ERROR)<<"redis cluster "<<clusterInfo.clusterId<<", bak redis cluster "<<bakClusterInfo.clusterId<<" doRedisCommand failed";
						bakIter++;
						continue;
					}
					else
					{
						if(parseScanKeysReply(replyInfo, keys, retCursor))
							updateClusterCursor(clusterId, retCursor);
						freeReplyInfo(replyInfo);
						if(keys.size()>=count)
						{
//							m_logger.debug("get enough %d keys", count);
							LOG(INFO)<<"get enough "<<count<<" keys";
							break;
						}
						else if(retCursor!=0)
						{
							// continue scan this bak cluster
							continue;
						}
					}					
				}
			}
			else
			{
				if(parseScanKeysReply(replyInfo, keys, retCursor))
					updateClusterCursor(clusterId, retCursor);
				freeReplyInfo(replyInfo);
				if(keys.size()>=count)
				{
//					m_logger.debug("get enough %d keys");
					LOG(INFO)<<"get enough "<<count<<" keys";
					break;
				}
				else if(retCursor!=0)
				{
					// continue scan this cluster
					continue;
				}
			}
		}
		else
		{
//			m_logger.debug("redis %s is not alive master, isMaster %d, isAlive %d", clusterId.c_str(), clusterInfo.isMaster, clusterInfo.isAlived);	
			LOG(INFO)<<"redis "<<clusterId<<" is not alive master, isMaster "<<clusterInfo.isMaster<<", isAlive "<<clusterInfo.isAlived;
		}
		if(keys.size()>=count)
		{
//			m_logger.debug("get enough %d keys");
			LOG(INFO)<<"get enough "<<count<<" keys";
			break;
		}
		clusterIter++; // scan next cluster
	}	
	freeCommandList(paraList);
//	m_logger.debug("scan keys get %d keys", keys.size());
	LOG(INFO)<<"scan keys get "<<keys.size()<<" keys";
	return keys.size()>0 ? true : false;
}

bool RedisClient::getKeys(const string & queryKey,list < string > & keys)
{
	//need send to all master cluster
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("keys", 4, paraList);
	paraLen += 15;
	string key = queryKey + "*";
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	//
	REDIS_CLUSTER_MAP clusterMap;
	getRedisClusters(clusterMap);
	REDIS_CLUSTER_MAP::iterator clusterIter;
	for (clusterIter = clusterMap.begin(); clusterIter != clusterMap.end(); clusterIter++)
	{
		string clusterId = (*clusterIter).first;
		RedisClusterInfo clusterInfo;
		if(!getRedisClusterInfo(clusterId, clusterInfo))
		{
//			m_logger.warn("get redis cluster:[%s] info.", clusterId.c_str());
			LOG(WARNING)<<"get redis cluster:["<<clusterId<<"] info.";
			continue;
		}
		if (clusterInfo.isMaster && clusterInfo.isAlived)
		{
			RedisReplyInfo replyInfo;
			if(!clusterInfo.clusterHandler->doRedisCommand(paraList, paraLen, replyInfo))
			{
				freeReplyInfo(replyInfo);
				//need send to backup cluster.
				if (clusterInfo.bakClusterList.size() != 0)
				{
					bool sendBak = false;
					list<string>::iterator bakIter;
					for (bakIter = clusterInfo.bakClusterList.begin(); bakIter != clusterInfo.bakClusterList.end(); bakIter++)
					{
						RedisClusterInfo bakClusterInfo;
						if(!getRedisClusterInfo((*bakIter), bakClusterInfo))
						{
//							m_logger.warn("get bak redis cluster:[%s] info.", (*bakIter).c_str());
							LOG(WARNING)<<"get bak redis cluster:["<<*bakIter<<"] info.";
							continue;
						}
						if(!bakClusterInfo.clusterHandler->doRedisCommand(paraList, paraLen, replyInfo))
						{
							freeReplyInfo(replyInfo);
							continue;
						}
						else
						{
							sendBak = true;
						}
					}
					if (!sendBak)
					{
						continue;
					}
				}
			}
			parseGetKeysReply(replyInfo, keys);
			freeReplyInfo(replyInfo);
		}
	}
	freeCommandList(paraList);
	return true;
}

void RedisClient::getRedisClusters(REDIS_CLUSTER_MAP & clusterMap)
{
	ReadGuard guard(m_rwClusterMutex);
	clusterMap = m_clusterMap;
	return;
}

bool RedisClient::getRedisClustersByCommand(REDIS_CLUSTER_MAP & clusterMap)
{
	assert(m_redisMode==CLUSTER_MODE);
	bool success = false;
	//send cluster nodes.
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("cluster", 7, paraList);
	fillCommandPara("nodes", 5, paraList);
	paraLen += 30;
	REDIS_CLUSTER_MAP oldClusterMap;
	getRedisClusters(oldClusterMap);
	REDIS_CLUSTER_MAP::iterator clusterIter;
	for (clusterIter = oldClusterMap.begin(); clusterIter != oldClusterMap.end(); clusterIter++)
	{
		RedisClusterInfo oldClusterInfo;
		string clusterId = (*clusterIter).first;
		if(!getRedisClusterInfo(clusterId, oldClusterInfo))
		{
//			m_logger.warn("get cluster:%s info failed.", clusterId.c_str());
			LOG(ERROR)<<"get cluster:"<<clusterId<<" info failed.";
			continue;
		}
		RedisReplyInfo replyInfo;
		if (!oldClusterInfo.clusterHandler->doRedisCommand(paraList, paraLen, replyInfo))
		{
//			m_logger.warn("do get cluster nodes failed.");
			LOG(WARNING)<<"do get cluster nodes failed.";
			continue;
		}
		if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
		{
//			m_logger.warn("recv redis error response:[%s].", replyInfo.resultString.c_str());
			LOG(ERROR)<<"recv redis error response:["<<replyInfo.resultString<<"].";
			freeReplyInfo(replyInfo);
			freeCommandList(paraList);
			return false;
		}
		if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
		{
//			m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
			LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
			freeReplyInfo(replyInfo);
			freeCommandList(paraList);
			return false;
		}
		//check if master,slave parameter may be not newest.
		list<ReplyArrayInfo>::iterator arrayIter = replyInfo.arrayList.begin();
		string str = (*arrayIter).arrayValue;
		if (str.find("myself,slave") != string::npos)
		{
//			m_logger.info("the redis cluster:[%s] is slave. not use it reply,look for master.", (*clusterIter).first.c_str());
			LOG(WARNING)<<"the redis cluster:["<<(*clusterIter).first<<"] is slave. not use it reply,look for master.";
			freeReplyInfo(replyInfo);
			success = false;
			continue;
		}
		if (!parseClusterInfo(replyInfo, clusterMap))
		{
//			m_logger.error("parse cluster info from redis reply failed.");
			LOG(ERROR)<<"parse cluster info from redis reply failed.";
			freeCommandList(paraList);
			return false;
		}
		freeReplyInfo(replyInfo);
		success = true;
		break;
	}
	freeCommandList(paraList);
	return success;
}

bool RedisClient::checkAndSaveRedisClusters(REDIS_CLUSTER_MAP & clusterMap)
{
	WriteGuard guard(m_rwClusterMutex);
	REDIS_CLUSTER_MAP::iterator clusterIter;
	{
		WriteGuard guard(m_rwSlotMutex);
		m_slotMap.clear();
		for (clusterIter = clusterMap.begin(); clusterIter != clusterMap.end(); clusterIter++)
		{
			string clusterId = (*clusterIter).first;
			RedisClusterInfo clusterInfo = (*clusterIter).second;
			if (m_clusterMap.find(clusterId) != m_clusterMap.end())
			{
				RedisClusterInfo oldClusterInfo = m_clusterMap[clusterId];
				clusterInfo.clusterHandler = oldClusterInfo.clusterHandler;
				clusterInfo.connectionNum = oldClusterInfo.connectionNum;
				clusterInfo.connectTimeout = oldClusterInfo.connectTimeout;
				clusterInfo.readTimeout=oldClusterInfo.readTimeout;
				m_clusterMap[clusterId] = clusterInfo;
				if (clusterInfo.isMaster)
				{
					map<uint16_t,uint16_t>::iterator iter;
					for(iter = clusterInfo.slotMap.begin(); iter != clusterInfo.slotMap.end(); iter++)
					{
						uint16_t startSlotNum = (*iter).first;
						uint16_t stopSlotNum = (*iter).second;
						for(int i = startSlotNum; i <= stopSlotNum; i++)
						{
							m_slotMap[i] = clusterId;
						}
					}
				}
			}
			else
			{
				//need create new connect pool.
				clusterInfo.clusterHandler = new RedisCluster();
				clusterInfo.connectionNum = m_connectionNum;
				clusterInfo.connectTimeout = m_connectTimeout;
				if (!clusterInfo.clusterHandler->initConnectPool(clusterInfo.connectIp, clusterInfo.connectPort, clusterInfo.connectionNum, clusterInfo.connectTimeout, clusterInfo.readTimeout))
				{
//					m_logger.warn("init cluster:[%s] connect pool failed.", clusterId.c_str());
					LOG(ERROR)<<"init cluster:["<<clusterId<<"] connect pool failed.";
					return false;
				}
				m_clusterMap[clusterId] = clusterInfo;
				if (clusterInfo.isMaster)
				{
					map<uint16_t,uint16_t>::iterator iter;
					for(iter = clusterInfo.slotMap.begin(); iter != clusterInfo.slotMap.end(); iter++)
					{
						uint16_t startSlotNum = (*iter).first;
						uint16_t stopSlotNum = (*iter).second;
						for(int i = startSlotNum; i <= stopSlotNum; i++)
						{
							m_slotMap[i] = clusterId;
						}
					}
				}
			}
		}
	}
	//check old cluster map if need free.
	list<string> unusedClusters;
	unusedClusters.clear();
	for (clusterIter = m_clusterMap.begin(); clusterIter != m_clusterMap.end(); clusterIter++)
	{
		string clusterId = (*clusterIter).first;
		RedisClusterInfo clusterInfo = (*clusterIter).second;
		if (clusterMap.find(clusterId) == clusterMap.end())
		{
			m_unusedHandlers[clusterId] = clusterInfo.clusterHandler;
			unusedClusters.push_back(clusterId);
		}
	}
	list<string>::iterator unusedIter;
	for (unusedIter = unusedClusters.begin(); unusedIter != unusedClusters.end(); unusedIter++)
	{
		m_clusterMap.erase((*unusedIter));
	}
	return true;
}

bool RedisClient::setKeyExpireTime(const string & key,uint32_t expireTime)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("expire", 6, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	fillCommandPara(toStr(expireTime).c_str(),toStr(expireTime).length(), paraList);
	paraLen += toStr(expireTime).length() + 20;
	bool success = false;
	success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_EXPIRE);
	freeCommandList(paraList);
	return success;
}

void RedisClient::releaseUnusedClusterHandler()
{
	WriteGuard guard(m_rwClusterMutex);
	list<string> needFreeClusters;
	needFreeClusters.clear();
	map<string, RedisCluster*>::iterator clusterIter;
	for(clusterIter = m_unusedHandlers.begin(); clusterIter != m_unusedHandlers.end(); clusterIter++)
	{
		if(((*clusterIter).second)->checkIfCanFree())
		{
			needFreeClusters.push_back((*clusterIter).first);
		}
	}
	list<string>::iterator iter;
	for (iter = needFreeClusters.begin(); iter != needFreeClusters.end(); iter++)
	{
		RedisCluster* clusterHandler = m_unusedHandlers[(*iter)];
		clusterHandler->freeConnectPool();
		delete clusterHandler;
		clusterHandler = NULL;
		m_unusedHandlers.erase((*iter));
	}
}

bool RedisClient::parseClusterInfo(RedisReplyInfo & replyInfo,REDIS_CLUSTER_MAP & clusterMap)
{
	//get reply string
	list<ReplyArrayInfo>::iterator iter = replyInfo.arrayList.begin();
	if (iter == replyInfo.arrayList.end())
	{
//		m_logger.warn("reply not have array info.");
		LOG(ERROR)<<"reply not have array info.";
		return false;
	}
	if ((*iter).replyType != RedisReplyType::REDIS_REPLY_STRING || (*iter).arrayLen == 0)
	{
//		m_logger.error("parse cluster info failed.redis reply info is something wrong,replyType:[%d], arrayLen:[%d].", (*iter).replyType, (*iter).arrayLen);
		LOG(ERROR)<<"parse cluster info failed.redis reply info is wrong,replyType:["<<(*iter).replyType<<"], arrayLen:["<<(*iter).arrayLen<<"].";
		return false;
	}
//	m_logger.debug("recv redis cluster response:[%s].", (*iter).arrayValue);
	LOG(INFO)<<"recv redis cluster response:["<<(*iter).arrayValue<<"].";
	string str = (*iter).arrayValue;
	string::size_type startPos,findPos;
	startPos = findPos = 0;
	findPos = str.find("\n", startPos);
	map<string, string> bakMap; //for key is bak cluster address info, value is master cluster id.
	bakMap.clear();
	while (findPos != string::npos)
	{
		string infoStr = str.substr(startPos, findPos-startPos);
		if (infoStr == "\r" )
			break;
		if(	infoStr.find("fail") != string::npos
			|| infoStr.find("noaddr") != string::npos
			|| infoStr.find("disconnected") != string::npos )
		{
			startPos = findPos + 1;
			findPos = str.find("\n", startPos);		
			continue;
		}
		RedisClusterInfo clusterInfo;
		if (!parseOneCluster(infoStr,clusterInfo))
		{
//			m_logger.warn("parse one cluster failed.");
			LOG(ERROR)<<"parse one cluster failed.";
			clusterMap.clear();
			return false;
		}
		string clusterId = clusterInfo.connectIp + ":" + toStr(clusterInfo.connectPort);
		//check if bak cluster node info.
		if (!clusterInfo.isMaster && !clusterInfo.masterClusterId.empty())
		{
			bakMap[clusterId] = clusterInfo.masterClusterId;
		}
		clusterInfo.connectionNum = m_connectionNum;
		clusterInfo.connectTimeout = m_connectTimeout;
		clusterInfo.readTimeout=m_readTimeout;
		clusterMap[clusterId] = clusterInfo;
		startPos = findPos + 1;
		findPos = str.find("\n", startPos);
	}
	//
	map<string, string>::iterator bakIter;
	for (bakIter = bakMap.begin(); bakIter != bakMap.end(); bakIter++)
	{
		REDIS_CLUSTER_MAP::iterator clusterIter;
		for (clusterIter = clusterMap.begin(); clusterIter != clusterMap.end(); clusterIter++)
		{
			if (((*clusterIter).second).clusterId == (*bakIter).second)
			{
				((*clusterIter).second).bakClusterList.push_back((*bakIter).first);
			}
		}
	}
	return true;
}

bool RedisClient::parseOneCluster(const string & infoStr,RedisClusterInfo & clusterInfo)
{
	//first parse node id.
	string::size_type startPos, findPos;
	startPos = findPos = 0;
	findPos = infoStr.find(" ");
	if (findPos != string::npos)
	{
		clusterInfo.clusterId = infoStr.substr(0, findPos);
		startPos = findPos + 1;
		findPos = infoStr.find(" ", startPos);
		if (findPos == string::npos)
		{	
//			m_logger.warn("parse one cluster:[%s] failed.", infoStr.c_str());
			LOG(ERROR)<<"parse one cluster:["<<infoStr<<"] failed.";
			return false;
		}
		//parse ip port
		string address = infoStr.substr(startPos, findPos-startPos);
		startPos = findPos + 1;
		findPos = address.find(":");
		if (findPos != string::npos)
		{
			clusterInfo.connectIp = address.substr(0, findPos);
			clusterInfo.connectPort = atoi(address.substr(findPos+1, address.length()-findPos).c_str());
		}
		else
		{
			clusterInfo.connectIp = address;
			clusterInfo.connectPort = REDIS_DEFALUT_SERVER_PORT;
		}
		//parse master slave.
		findPos = infoStr.find(" ", startPos);
		if (findPos == string::npos)
		{
			return false;
		}
		string tmpStr;
		tmpStr = infoStr.substr(startPos, findPos-startPos);
		if (tmpStr.find("master") != string::npos)
		{
			clusterInfo.isMaster = true;
		}
		startPos = findPos + 1;
		findPos = infoStr.find(" ", startPos);
		if (findPos == string::npos)
		{
			return false;
		}
		if (!clusterInfo.isMaster)
		{
			clusterInfo.masterClusterId = infoStr.substr(startPos, findPos-startPos);
		}
		startPos = findPos + 1;
		//first find status.
		findPos = infoStr.find("disconnected", startPos);
		if (findPos != string::npos)
		{
			clusterInfo.isAlived = false;
		}
		else
		{
			clusterInfo.isAlived = true;
		}
		findPos = infoStr.find("connected", startPos);
		if (clusterInfo.isMaster && clusterInfo.isAlived)
		{
			startPos = findPos +1;
			findPos = infoStr.find(" ", startPos);
			if (findPos == string::npos)
				return false;
			startPos = findPos +1;
			findPos = infoStr.find(" ", startPos);
			string slotStr;
			uint16_t startSlotNum, stopSlotNum;
			while (findPos != string::npos)
			{
				slotStr = infoStr.substr(startPos, findPos);
				startSlotNum = stopSlotNum = -1;
				parseSlotStr(slotStr,startSlotNum, stopSlotNum);
				clusterInfo.slotMap[startSlotNum] = stopSlotNum;
				startPos = findPos +1;
//				m_logger.info("parse cluster slot success,cluster:[%s] has slot[%d-%d].", clusterInfo.clusterId.c_str(), startSlotNum, stopSlotNum);
				LOG(INFO)<<"parse cluster slot success,cluster:["<<clusterInfo.clusterId<<"] has slot["<<startSlotNum<<"-"<<stopSlotNum<<"].";
				findPos = infoStr.find(" ", startPos);
			}
			slotStr = infoStr.substr(startPos, infoStr.length()-startPos);
			startSlotNum = stopSlotNum = -1;
			parseSlotStr(slotStr,startSlotNum, stopSlotNum);
			clusterInfo.slotMap[startSlotNum] = stopSlotNum;
//			m_logger.info("parse cluster slot success,cluster:[%s] has slot[%d-%d].", clusterInfo.clusterId.c_str(), startSlotNum, stopSlotNum);
			LOG(INFO)<<"parse cluster slot success,cluster:["<<clusterInfo.clusterId<<"] has slot["<<startSlotNum<<"-"<<stopSlotNum<<"].";
		}
// 		m_logger.info("parse cluster:[%s] info success, cluster address:[%s:%d] master:[%d], active:[%d], master cluster id:[%s].", clusterInfo.clusterId.c_str(), clusterInfo.connectIp.c_str(), clusterInfo.connectPort, clusterInfo.isMaster, clusterInfo.isAlived, clusterInfo.masterClusterId.c_str());
 		LOG(INFO)<<"parse cluster:["<<clusterInfo.clusterId<<"] info success, cluster address:["<<clusterInfo.connectIp<<":"<<clusterInfo.connectPort<<"] master:["<<clusterInfo.isMaster<<"], active:["<<clusterInfo.isAlived<<"], master cluster id:["<<clusterInfo.masterClusterId<<"].";
	}
	return true;
}

void RedisClient::parseSlotStr(string & slotStr,uint16_t & startSlotNum,uint16_t & stopSlotNum)
{
	string::size_type findPos;
	findPos = slotStr.find("-");
	if (findPos != string::npos)
	{
		startSlotNum = atoi(slotStr.substr(0, findPos).c_str());
		stopSlotNum = atoi(slotStr.substr(findPos+1, slotStr.length()-findPos).c_str());
	}
}

// return true if get "MOVED" and redirectInfo, or false
bool RedisClient::checkIfNeedRedirect(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo)
{
	//assert(m_redisMode==CLUSTER_MODE);
	if(m_redisMode == STAND_ALONE_OR_PROXY_MODE)
	{
		return false;
	}
	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
	{
//		m_logger.warn("recv redis error response:[%s].", replyInfo.resultString.c_str());
		LOG(WARNING)<<"recv redis error response:["<<replyInfo.resultString<<"].";
		//check if move response.
		if (strncasecmp(replyInfo.resultString.c_str(), "MOVED", 5) == 0)
		{
			needRedirect = true;
			string::size_type findPos;
			findPos = replyInfo.resultString.find_last_of(" ");
			if (findPos != string::npos)
			{
				redirectInfo = replyInfo.resultString.substr(findPos+1, replyInfo.resultString.length()-findPos-1);
                //m_logger.info("redirectInfo is %s", redirectInfo.c_str());
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return false;
}

bool RedisClient::parseGetSerialReply(RedisReplyInfo & replyInfo,DBSerialize & serial,bool & needRedirect,string & redirectInfo)
{
	if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
	{
//		m_logger.info("need direct to cluster:[%s].", redirectInfo.c_str());
		LOG(INFO)<<"need direct to cluster:["<<redirectInfo<<"].";
		return true;
	}	
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STRING)
	{
//		m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}
	list<ReplyArrayInfo>::iterator iter = replyInfo.arrayList.begin();
	if (iter == replyInfo.arrayList.end())
	{
//		m_logger.warn("reply not have array info.");
		LOG(ERROR)<<"reply not have array info.";
		return false;
	}
	if ((*iter).replyType == RedisReplyType::REDIS_REPLY_NIL)
	{
//		m_logger.warn("get failed,the key not exist.");
		LOG(ERROR)<<"get failed,the key not exist.";
		return false;
	}
	DBInStream in((void*)(*iter).arrayValue, (*iter).arrayLen);
	serial.load(in);
	if (in.m_loadError)
	{
//		m_logger.warn("load data from redis error.");
		LOG(ERROR)<<"load data from redis error.";
		return false;
	}
	return true;	
}

bool RedisClient::parseFindReply(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo)
{
	if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
	{
//		m_logger.info("need direct to cluster:[%s].", redirectInfo.c_str());
		LOG(INFO)<<"need direct to cluster:["<<redirectInfo<<"].";
		return true;
	}
	if(replyInfo.replyType != RedisReplyType::REDIS_REPLY_INTEGER)
	{
		return false;
	}
	return true;
}

bool RedisClient::parseSetSerialReply(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo)
{
	if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
	{
//		m_logger.info("need direct to cluster:[%s].", redirectInfo.c_str());
		LOG(INFO)<<"need direct to cluster:["<<redirectInfo<<"].";
		return true;
	}	
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STATUS)
	{
//		m_logger.warn("set serial failed, redis response:[%s].", replyInfo.resultString.c_str());
		LOG(ERROR)<<"set serial failed, redis response:["<<replyInfo.resultString<<"].";
		return false;
	}
	return true;
}

bool RedisClient::parseScanKeysReply(RedisReplyInfo & replyInfo, list<string>& keys, int &retCursor)
{
	retCursor=-1;
//	m_logger.debug("parseScanKeysReply, replyInfo has replyType %d, resultString %s, intValue %d", replyInfo.replyType, replyInfo.resultString.c_str(), replyInfo.intValue);	
	LOG(INFO)<<"parseScanKeysReply, replyInfo has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_ARRAY)
	{
//		m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}

	retCursor=replyInfo.intValue;
	list<ReplyArrayInfo>::iterator arrayIter;
	for (arrayIter = replyInfo.arrayList.begin(); arrayIter != replyInfo.arrayList.end(); arrayIter++)
	{
//		m_logger.debug("arrayList has replyType %d, arrayValue %s, arrayLen %d", (*arrayIter).replyType, arrayIter->arrayValue, arrayIter->arrayLen);
		LOG(INFO)<<"arrayList has replyType "<<(*arrayIter).replyType<<", arrayValue "<<arrayIter->arrayValue<<", arrayLen "<<arrayIter->arrayLen;
		
		if ((*arrayIter).replyType == RedisReplyType::REDIS_REPLY_STRING)
		{
			string key = (*arrayIter).arrayValue;
			keys.push_back(key);
		}			
	}
	return true;
}

bool RedisClient::parseGetKeysReply(RedisReplyInfo & replyInfo,list < string > & keys)
{
//	m_logger.debug("replyInfo has replyType %d, resultString %s, intValue %d", replyInfo.replyType, replyInfo.resultString.c_str(), replyInfo.intValue);
	LOG(INFO)<<"replyInfo has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ERROR)
	{
//		m_logger.info("get empty list or set.");
		LOG(INFO)<<"get empty list or set.";
		return true;
	}

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_ARRAY)
	{
//		m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}
	
	list<ReplyArrayInfo>::iterator arrayIter;
	for (arrayIter = replyInfo.arrayList.begin(); arrayIter != replyInfo.arrayList.end(); arrayIter++)
	{
//		m_logger.debug("arrayList has replyType %d, arrayValue %s, arrayLen %d", (*arrayIter).replyType, arrayIter->arrayValue, arrayIter->arrayLen);
		LOG(INFO)<<"arrayList has replyType "<<(*arrayIter).replyType<<", arrayValue "<<arrayIter->arrayValue<<", arrayLen "<<arrayIter->arrayLen;
		
		if ((*arrayIter).replyType == RedisReplyType::REDIS_REPLY_STRING)
		{
			string key = (*arrayIter).arrayValue;
			keys.push_back(key);
		}
	}
	return true;
}

//for watch,unwatch,multi command response.
bool RedisClient::parseStatusResponseReply(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo)
{
//	m_logger.debug("replyInfo has replyType %d, resultString %s, intValue %d", replyInfo.replyType, replyInfo.resultString.c_str(), replyInfo.intValue);
	LOG(INFO)<<"replyInfo has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;
	
	if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
	{
//		m_logger.info("need direct to cluster:[%s].", redirectInfo.c_str());
		LOG(INFO)<<"need direct to cluster:["<<redirectInfo<<"].";
		return true;
	}		
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STATUS)
	{
//		m_logger.warn("status response failed, redis response:[%s].", replyInfo.resultString.c_str());
		LOG(ERROR)<<"status response failed, redis response:["<<replyInfo.resultString<<"].";
		return false;
	}
	return true;
}

bool RedisClient::parseExecReply(RedisReplyInfo & replyInfo,bool & needRedirect,string & redirectInfo)
{
//	m_logger.debug("replyInfo has replyType %d, resultString %s, intValue %d", replyInfo.replyType, replyInfo.resultString.c_str(), replyInfo.intValue);
	LOG(INFO)<<"replyInfo has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if (checkIfNeedRedirect(replyInfo, needRedirect, redirectInfo))
	{
//		m_logger.info("need direct to cluster:[%s].", redirectInfo.c_str());
		LOG(INFO)<<"need direct to cluster:["<<redirectInfo<<"].";
		return true;
	}

	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_ARRAY)
	{	
//		m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}
	//parse exec reply
	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ARRAY && replyInfo.intValue == -1)
	{	
//		m_logger.warn("exec reply -1,set serial exec failed.");
		LOG(ERROR)<<"exec reply -1,set serial exec failed.";
		return false;
	}
	//parse array list.
	list<ReplyArrayInfo>::iterator arrayIter;
	for (arrayIter = replyInfo.arrayList.begin(); arrayIter != replyInfo.arrayList.end(); arrayIter++)
	{
//		m_logger.debug("arrayList has replyType %d, arrayValue %s, arrayLen %d", (*arrayIter).replyType, arrayIter->arrayValue, arrayIter->arrayLen);
		LOG(INFO)<<"arrayList has replyType "<<(*arrayIter).replyType<<", arrayValue "<<arrayIter->arrayValue<<", arrayLen "<<arrayIter->arrayLen;
		if ((*arrayIter).replyType == RedisReplyType::REDIS_REPLY_STRING)
		{
			if (strncmp((*arrayIter).arrayValue, "-", 1) == 0 || strncmp((*arrayIter).arrayValue, ":0", 2) == 0)
			{
//				m_logger.warn("recv failed exec reply:%s.", (*arrayIter).arrayValue);
				LOG(ERROR)<<"recv failed exec reply: "<<(*arrayIter).arrayValue;
				return false;
			}
//			m_logger.debug("recv exec reply:%s.", (*arrayIter).arrayValue);
			LOG(INFO)<<"recv exec reply: "<<(*arrayIter).arrayValue;
		}
	}
	return true;
}

void RedisClient::freeReplyInfo(RedisReplyInfo & replyInfo)
{
	if (replyInfo.arrayList.size() > 0)
	{
		list<ReplyArrayInfo>::iterator iter;
		for (iter = replyInfo.arrayList.begin(); iter != replyInfo.arrayList.end(); iter++)
		{
			if ((*iter).arrayValue != NULL)
			{
				free((*iter).arrayValue);
				(*iter).arrayValue = NULL;
			}
		}
		replyInfo.arrayList.clear();
	}
}

void RedisClient::freeReplyInfo(CommonReplyInfo & replyInfo)
{
	for(size_t i=0; i<replyInfo.arrays.size(); i++)
	{
		for(size_t j=0; j<replyInfo.arrays[i].size(); j++)
		{
			if (replyInfo.arrays[i][j].arrayValue != NULL)
			{
				free(replyInfo.arrays[i][j].arrayValue);
				replyInfo.arrays[i][j].arrayValue = NULL;
			}
		}
	}
	replyInfo.arrays.clear();
//	if (replyInfo.arrayList.size() > 0)
//	{
//		list<ReplyArrayInfo>::iterator iter;
//		for (iter = replyInfo.arrayList.begin(); iter != replyInfo.arrayList.end(); iter++)
//		{
//			if ((*iter).arrayValue != NULL)
//			{
//				free((*iter).arrayValue);
//				(*iter).arrayValue = NULL;
//			}
//		}
//		replyInfo.arrayList.clear();
//	}
}


void RedisClient::fillCommandPara(const char * paraValue,int32_t paraLen,list < RedisCmdParaInfo > & paraList)
{
//	m_logger.debug("fillCommandPara : paraValue %s, paraLen %d", paraValue, paraLen);
	LOG(INFO)<<"fillCommandPara : paraValue "<<paraValue<<", paraLen "<<paraLen;
	
	RedisCmdParaInfo paraInfo;
	paraInfo.paraLen = paraLen;
	paraInfo.paraValue = (char*)malloc(paraLen+1);
	memset(paraInfo.paraValue, 0, paraLen+1);
	memcpy(paraInfo.paraValue, paraValue, paraLen);
	paraList.push_back(paraInfo);
}

void RedisClient::freeCommandList(list < RedisCmdParaInfo > & paraList)
{
	list<RedisCmdParaInfo>::iterator commandIter;
	for (commandIter = paraList.begin(); commandIter != paraList.end(); commandIter++)
	{
		free((*commandIter).paraValue);
		(*commandIter).paraValue = NULL;
	}
	paraList.clear();
}

bool RedisClient::getRedisClusterInfo(string & clusterId,RedisClusterInfo & clusterInfo)
{
	ReadGuard guard(m_rwClusterMutex);
	if (m_clusterMap.find(clusterId) != m_clusterMap.end())
	{
		clusterInfo = m_clusterMap[clusterId];
		return true;
	}
//	m_logger.warn("not find cluster:%s info.", clusterId.c_str());
	LOG(ERROR)<<"not find cluster:"<<clusterId<<" info.";
	return false;
}

void RedisClient::updateClusterCursor(const string& clusterId, int newcursor)
{
	if(newcursor<0)
		return;
	ReadGuard guard(m_rwClusterMutex);
	if (m_clusterMap.find(clusterId) != m_clusterMap.end())
	{
		m_clusterMap[clusterId].scanCursor=newcursor;
		return;
	}
//	m_logger.warn("updateClusterCursor non-exist redis cluster %s", clusterId.c_str());
	LOG(ERROR)<<"updateClusterCursor non-exist redis cluster "<<clusterId;
}


bool RedisClient::getClusterIdBySlot(uint16_t slotNum,string & clusterId)
{
	assert(m_redisMode==CLUSTER_MODE);
	ReadGuard guard(m_rwSlotMutex);
	if (m_slotMap.find(slotNum) != m_slotMap.end())
	{
		clusterId = m_slotMap[slotNum];
//		m_logger.info("slot:%u hit clusterId:%s.", slotNum, clusterId.c_str());
		LOG(INFO)<<"slot:"<<slotNum<<" hit clusterId: "<<clusterId;
		return true;
	}
//	m_logger.warn("slot:%u not hit any cluster.please check redis cluster.", slotNum);
	LOG(ERROR)<<"slot:"<<slotNum<<" not hit any cluster.please check redis cluster.";
	return false;
}

bool RedisClient::zadd(const string& key,const string& member, int score)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara("zadd", 4, paraList);
    paraLen += 16;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;
    
    char score_c[64] = {0};
    sprintf(score_c,"%d",score);
    fillCommandPara(score_c, strlen(score_c), paraList);
    paraLen += strlen(score_c) + 20;
    
    fillCommandPara(member.c_str(), member.length(), paraList);
    paraLen += member.length() + 20;

    bool success = false;
    success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_ZADD);
    freeCommandList(paraList);
    return success;
}

bool RedisClient::zrem(const string& key,const string& member)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara("zrem", 4, paraList);
    paraLen += 16;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;
    fillCommandPara(member.c_str(), member.length(), paraList);
    paraLen += member.length() + 20;

    bool success = false;
    success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_ZREM);
    freeCommandList(paraList);
    return success;
}

bool RedisClient::zincby(const string& key,const string& member,int increment)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara("zincrby", 7, paraList);
    paraLen += 19;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;
    
    char inc[64] = {0};
    sprintf(inc,"%d",increment);
    fillCommandPara(inc, strlen(inc), paraList);
    paraLen += strlen(inc) + 20;   
    
    fillCommandPara(member.c_str(), member.length(), paraList);
    paraLen += member.length() + 20;

    bool success = false;
    success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_ZINCRBY);
    freeCommandList(paraList);
    return success;
}

int RedisClient::zcount(const string& key,int start, int end)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara("zcount", 6, paraList);
    paraLen += 18;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;

    char start_c[64] = {0};
    sprintf(start_c,"%d",start);
    fillCommandPara(start_c, strlen(start_c), paraList);
    paraLen += strlen(start_c) + 20; 
    char end_c[64] = {0};
    sprintf(end_c,"%d",end);
    fillCommandPara(end_c, strlen(end_c), paraList);
    paraLen += strlen(end_c) + 20;

    int count = 0;
    bool success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_ZCOUNT, &count);
    (void)success;
    freeCommandList(paraList);
    return count;
}

int RedisClient::zcard(const string& key)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="zcard";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;

    int count = 0;
    bool success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_ZCARD, &count);
    (void) success;
    freeCommandList(paraList);
    return count;
}

int RedisClient::dbsize()
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="DBSIZE";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;

    int count = 0;
    bool success = doRedisCommand( "",paraLen,paraList,RedisCommandType::REDIS_COMMAND_DBSIZE, &count);
    (void)success;
    freeCommandList(paraList);
    return count;
}

int RedisClient::zscore(const string& key,const string& member)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara("zscore", 6, paraList);
    paraLen += 18;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;
    fillCommandPara(member.c_str(), member.length(), paraList);
    paraLen += member.length() + 20;

    int count = -1;
    bool success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_ZSCORE, &count);
    (void)success;
    freeCommandList(paraList);
    return count;
}

bool RedisClient::zrangebyscore(const string& key,int start, int end, list<string>& members)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara("zrangebyscore", 13, paraList);
    paraLen += 25;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;

    char start_c[64] = {0};
    sprintf(start_c,"%d",start);
    fillCommandPara(start_c, strlen(start_c), paraList);
    paraLen += strlen(start_c) + 20; 
    char end_c[64] = {0};
    sprintf(end_c,"%d",end);
    fillCommandPara(end_c, strlen(end_c), paraList);
    paraLen += strlen(end_c) + 20;

    bool success = false;
    success = doRedisCommand( key,paraLen,paraList, RedisCommandType::REDIS_COMMAND_ZRANGEBYSCORE, members);
    freeCommandList(paraList);
    return success;
}

bool RedisClient::zremrangebyscore(const string& key,int start, int end)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    fillCommandPara("zremrangebyscore", 16, paraList);
    paraLen += 28;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;

    char start_c[64] = {0};
    sprintf(start_c,"%d",start);
    fillCommandPara(start_c, strlen(start_c), paraList);
    paraLen += strlen(start_c) + 20; 
    char end_c[64] = {0};
    sprintf(end_c,"%d",end);
    fillCommandPara(end_c, strlen(end_c), paraList);
    paraLen += strlen(end_c) + 20;

    bool success = false;
    success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_ZREMRANGEBYSCORE);
    freeCommandList(paraList);
    return success;
}


// sadd  key member [member...]
bool RedisClient::sadd(const string& key, const string& member)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="sadd";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;
 
    fillCommandPara(member.c_str(), member.length(), paraList);
    paraLen += member.length() + 20;

    bool success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_SADD);
    freeCommandList(paraList);
    return success;
}

bool RedisClient::srem(const string& key, const string& member)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="srem";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;

    fillCommandPara(member.c_str(), member.length(), paraList);
    paraLen += member.length() + 20;

    bool success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_SREM);
    freeCommandList(paraList);
    return success;
}

int RedisClient::scard(const string& key)
{
    list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="scard";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;

    int count = 0;
    bool success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_SCARD, &count);
    (void)success;
    freeCommandList(paraList);
    return count;
}

bool RedisClient::sismember(const string& key, const string& member)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="sismember";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 20;
   
    fillCommandPara(member.c_str(), member.length(), paraList);
    paraLen += member.length() + 20;

    int count = -1;
    bool success = doRedisCommand( key,paraLen,paraList,RedisCommandType::REDIS_COMMAND_SISMEMBER, &count);
    (void) success;
    freeCommandList(paraList);
    return (count==1) ? true : false;
}

bool RedisClient::smembers(const string& key, list<string>& members)
{
	list<RedisCmdParaInfo> paraList;
    int32_t paraLen = 0;
    string str="smembers";
    fillCommandPara(str.c_str(), str.size(), paraList);
    paraLen += str.size()+11;
    fillCommandPara(key.c_str(), key.length(), paraList);
    paraLen += key.length() + 11;

    bool success = false;
    success = doRedisCommand( key,paraLen,paraList, RedisCommandType::REDIS_COMMAND_SMEMBERS, members);
    freeCommandList(paraList);
    return success;
}

/*
	API for transaction of STAND_ALONE_OR_PROXY_MODE
*/
bool RedisClient::PrepareTransaction(RedisConnection** conn)
{
    if(m_redisMode!=STAND_ALONE_OR_PROXY_MODE)
    {
		LOG(ERROR)<<"transaction only supported in standalone mode";
        return false;
    }
    
    RedisConnection* con=m_redisProxy.clusterHandler->getAvailableConnection();
    if(con==NULL)
    {
//        m_logger.error("cannot acquire a redis connection");
		LOG(ERROR)<<"cannot acquire a redis connection";
        return false;
    }
    con->SetCanRelease(false);
    *conn=con;
	return true;
}

bool RedisClient::WatchKeys(const vector<string>& keys, RedisConnection* con)
{
	for(size_t i=0; i<keys.size(); i++)
	{
		if(!WatchKey(keys[i], con))
			return false;
	}
	return true;
}

bool RedisClient::WatchKey(const string& key, RedisConnection* con)
{
	list<RedisCmdParaInfo> watchParaList;
	int32_t watchParaLen = 0;
	fillCommandPara("watch", 5, watchParaList);
	watchParaLen += 18;
	fillCommandPara(key.c_str(), key.length(), watchParaList);
	watchParaLen += key.length() + 20;
	bool success = doTransactionCommandInConnection(watchParaLen,watchParaList,RedisCommandType::REDIS_COMMAND_WATCH, con);
	freeCommandList(watchParaList);
    return success;
}

bool RedisClient::Unwatch(RedisConnection* con)
{
    list<RedisCmdParaInfo> unwatchParaList;
    int32_t unwatchParaLen = 0;
    fillCommandPara("unwatch", 6, unwatchParaList);
    unwatchParaLen += 20;
	bool success = doTransactionCommandInConnection(unwatchParaLen,unwatchParaList,RedisCommandType::REDIS_COMMAND_UNWATCH, con);
    freeCommandList(unwatchParaList);
    return success;
}


bool RedisClient::StartTransaction(RedisConnection* con)
{
    if(m_redisMode!=STAND_ALONE_OR_PROXY_MODE)
    {
		LOG(ERROR)<<"transaction only supported in standalone mode";
        return false;
    }

	bool success = false;
	list<RedisCmdParaInfo> multiParaList;
	int32_t multiParaLen = 0;
	fillCommandPara("multi", 5, multiParaList);
	multiParaLen += 18;
	
	success = doTransactionCommandInConnection(multiParaLen,multiParaList,RedisCommandType::REDIS_COMMAND_MULTI, con);
	freeCommandList(multiParaList);
	if (!success)
	{
//		m_logger.warn("do multi command failed.");
		LOG(ERROR)<<"do multi command failed.";
    }
    return success;
}


bool RedisClient::DiscardTransaction(RedisConnection* con)
{
    if(m_redisMode!=STAND_ALONE_OR_PROXY_MODE)
    {
		LOG(ERROR)<<"transaction only supported in standalone mode";
        return false;
    }

	bool success = false;
	list<RedisCmdParaInfo> paraList;
	int32_t multiParaLen = 0;
	fillCommandPara("discard", 7, paraList);
	multiParaLen += 18;
	success = doTransactionCommandInConnection(multiParaLen,paraList,RedisCommandType::REDIS_COMMAND_DISCARD, con);
	freeCommandList(paraList);
	if (!success)
	{
//		m_logger.warn("do discard command failed.");
		LOG(ERROR)<<"do discard command failed.";
    }
    return success;
}


bool RedisClient::ExecTransaction(RedisConnection* con)
{
    if(m_redisMode!=STAND_ALONE_OR_PROXY_MODE)
    {
		LOG(ERROR)<<"transaction only supported in standalone mode";
        return false;
    }

	bool success = false;
	list<RedisCmdParaInfo> paraList;
	int32_t multiParaLen = 0;
	fillCommandPara("exec", 4, paraList);
	multiParaLen += 18;
	success = doTransactionCommandInConnection(multiParaLen,paraList,RedisCommandType::REDIS_COMMAND_EXEC, con);
	freeCommandList(paraList);
	if (!success)
	{
//		m_logger.warn("exec transaction failed.");
		LOG(ERROR)<<"exec transaction failed.";
		return false;
    }
    else
    {
//	    m_logger.debug("exec transaction ok");
		LOG(INFO)<<"exec transaction ok";
	    return true;
	}
}


bool RedisClient::FinishTransaction(RedisConnection** conn)
{
    if(m_redisMode!=STAND_ALONE_OR_PROXY_MODE)
    {
		LOG(ERROR)<<"transaction only supported in standalone mode";
        return false;
    }

	if(conn==NULL  ||  *conn==NULL)
		return false;
    m_redisProxy.clusterHandler->releaseConnection(*conn);
    *conn=NULL;
	return true;
}


bool RedisClient::doTransactionCommandInConnection(int32_t commandLen, list<RedisCmdParaInfo> &commandList, int commandType, RedisConnection* con)
{
	RedisReplyInfo replyInfo;
	bool needRedirect;
	string redirectInfo;
    if(m_redisProxy.clusterHandler==NULL)
    {
//        m_logger.error("m_redisProxy.clusterHandler is NULL");
		LOG(ERROR)<<"m_redisProxy.clusterHandler is NULL";
        return false;
    }
	if(!m_redisProxy.clusterHandler->doRedisCommandOneConnection(commandList, commandLen, replyInfo, false, &con))
	{
		freeReplyInfo(replyInfo);
//		m_logger.warn("proxy:%s do redis command failed.", m_redisProxy.proxyId.c_str());
		LOG(ERROR)<<"proxy:"<<m_redisProxy.proxyId<<" do redis command failed.";
		return false;
	}

	switch (commandType)
	{
		// expects "QUEUED"			
		case RedisCommandType::REDIS_COMMAND_SET:		
		case RedisCommandType::REDIS_COMMAND_DEL:		
		case RedisCommandType::REDIS_COMMAND_SADD:
		case RedisCommandType::REDIS_COMMAND_SREM:
			if(!parseQueuedResponseReply(replyInfo))
			{
//				m_logger.warn("parse queued command reply failed. reply string:%s.", replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse queued command reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		// expects "OK"
		case RedisCommandType::REDIS_COMMAND_WATCH:
		case RedisCommandType::REDIS_COMMAND_UNWATCH:
		case RedisCommandType::REDIS_COMMAND_MULTI:
		case RedisCommandType::REDIS_COMMAND_DISCARD:
			if(!parseStatusResponseReply(replyInfo,needRedirect,redirectInfo))
			{
//				m_logger.warn("parse watch reply failed. reply string:%s.", replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse watch reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		// expects array
		case RedisCommandType::REDIS_COMMAND_EXEC:
			if(!parseExecReply(replyInfo))
			{
//				m_logger.warn("parse exec reply failed. reply string:%s.", replyInfo.resultString.c_str());
				LOG(ERROR)<<"parse exec reply failed. reply string: "<<replyInfo.resultString;
				freeReplyInfo(replyInfo);
				return false;
			}
			break;
		default:
//			m_logger.warn("recv unknown command type:%d", commandType);
			LOG(ERROR)<<"recv unknown command type: "<<commandType;
			return false;
	}
	freeReplyInfo(replyInfo);
	return true;
}

//for watch,unwatch,multi,discard command response.
bool RedisClient::parseStatusResponseReply(RedisReplyInfo & replyInfo)
{
//	m_logger.debug("status replyInfo has replyType %d, resultString %s, intValue %d", replyInfo.replyType, replyInfo.resultString.c_str(), replyInfo.intValue);
	LOG(INFO)<<"status replyInfo has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;
			
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STATUS)
	{
//		m_logger.warn("status response failed, redis response:[%s].", replyInfo.resultString.c_str());
		LOG(ERROR)<<"status response failed, redis response:["<<replyInfo.resultString<<"].";
		return false;
	}
	return true;
}

//for queued command in a transaction
bool RedisClient::parseQueuedResponseReply(RedisReplyInfo & replyInfo)
{
//	m_logger.debug("queued replyInfo has replyType %d, resultString %s, intValue %d", replyInfo.replyType, replyInfo.resultString.c_str(), replyInfo.intValue);
	LOG(INFO)<<"queued replyInfo has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;
			
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_STATUS)
	{
//		m_logger.warn("status response failed, redis response:[%s].", replyInfo.resultString.c_str());
		LOG(ERROR)<<"status response failed, redis response:["<<replyInfo.resultString<<"].";
		return false;
	}
	return true;
}

bool RedisClient::parseExecReply(RedisReplyInfo & replyInfo)
{
//	m_logger.debug("exec replyInfo has replyType %d, resultString %s, intValue %d", replyInfo.replyType, replyInfo.resultString.c_str(), replyInfo.intValue);
	LOG(INFO)<<"exec replyInfo has replyType "<<replyInfo.replyType<<", resultString "<<replyInfo.resultString<<", intValue "<<replyInfo.intValue;

	if(replyInfo.replyType  == RedisReplyType::REDIS_REPLY_NIL)
	{
//		m_logger.warn("transaction interrupted: nil");
		LOG(ERROR)<<"transaction interrupted: nil";
		return false;
	}
	if (replyInfo.replyType != RedisReplyType::REDIS_REPLY_ARRAY)
	{
//		m_logger.warn("recv redis wrong reply type:[%d].", replyInfo.replyType);
		LOG(ERROR)<<"recv redis wrong reply type:["<<replyInfo.replyType<<"].";
		return false;
	}
	// empty array
	if (replyInfo.replyType == RedisReplyType::REDIS_REPLY_ARRAY && replyInfo.intValue == -1)
	{
//		m_logger.warn("exec reply -1, set serial exec failed.");
		LOG(ERROR)<<"exec reply -1, set serial exec failed.";
		return false;
	}

	list<ReplyArrayInfo>::iterator arrayIter;
	for (arrayIter = replyInfo.arrayList.begin(); arrayIter != replyInfo.arrayList.end(); arrayIter++)
	{
//		m_logger.debug("arrayList has replyType %d, arrayValue %s, arrayLen %d", (*arrayIter).replyType, arrayIter->arrayValue, arrayIter->arrayLen);
		LOG(INFO)<<"arrayList has replyType "<<(*arrayIter).replyType<<", arrayValue "<<arrayIter->arrayValue<<", arrayLen "<<arrayIter->arrayLen;

		if ((*arrayIter).replyType == RedisReplyType::REDIS_REPLY_STRING)
		{
			// error type
			if (strncmp((*arrayIter).arrayValue, "-", 1) == 0)
			{
//				m_logger.warn("recv failed exec reply:%s.", (*arrayIter).arrayValue);
				LOG(ERROR)<<"recv failed exec reply: "<<(*arrayIter).arrayValue;
				return false;
			}
			// integer type: 0
			else if(strncmp((*arrayIter).arrayValue, ":0", 2) == 0)
			{
//				m_logger.warn("recv failed exec reply:%s.", (*arrayIter).arrayValue);
				LOG(WARNING)<<"recv failed exec reply: "<<(*arrayIter).arrayValue;
//				return false;
				return true;
			}
			// bulk string: nil
			else if(strncmp((*arrayIter).arrayValue, "$-1", 3) == 0)
			{
//				m_logger.warn("recv failed exec reply:%s.", (*arrayIter).arrayValue);
				LOG(ERROR)<<"recv failed exec reply: "<<(*arrayIter).arrayValue;
				return false;
			}
			// array type: empty array
			else if(strncmp((*arrayIter).arrayValue, "*-1", 3) == 0)
			{
//				m_logger.warn("recv failed exec reply:%s.", (*arrayIter).arrayValue);
				LOG(ERROR)<<"recv failed exec reply: "<<(*arrayIter).arrayValue;
				return false;
			}
		}
	}
	return true;
}

bool RedisClient::Set(RedisConnection* con, const string & key, const DBSerialize& serial)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("set", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;	
	DBOutStream out; 
	serial.save(out);
	fillCommandPara(out.getData(), out.getSize(), paraList);
	paraLen += out.getSize() + 20;
	bool success = doTransactionCommandInConnection(paraLen,paraList,RedisCommandType::REDIS_COMMAND_SET, con);
	freeCommandList(paraList);
	if(!success)
	{
//		m_logger.error("set of transaction failed");
		LOG(ERROR)<<"set of transaction failed";
	}
	return success;
}

bool RedisClient::Set(RedisConnection* con, const string & key, const string& value)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("set", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	fillCommandPara(value.c_str(), value.size(), paraList);
	paraLen += value.size() + 20;
	bool success = doTransactionCommandInConnection(paraLen,paraList,RedisCommandType::REDIS_COMMAND_SET, con);
	freeCommandList(paraList);
	if(!success)
	{
//		m_logger.error("set of transaction failed");
		LOG(ERROR)<<"set of transaction failed";
	}
	return success;
}

bool RedisClient::Del(RedisConnection* con, const string & key)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("del", 3, paraList);
	paraLen += 15;
	fillCommandPara(key.c_str(), key.length(), paraList);
	paraLen += key.length() + 20;
	bool success = doTransactionCommandInConnection(paraLen,paraList,RedisCommandType::REDIS_COMMAND_DEL, con);
	freeCommandList(paraList);
	if(!success)
	{
//		m_logger.error("del of transaction failed");
		LOG(ERROR)<<"del of transaction failed";
	}
	return success;
}

bool RedisClient::Sadd(RedisConnection* con, const string & setKey, const string& member)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("sadd", 4, paraList);
	paraLen += 15;
	fillCommandPara(setKey.c_str(), setKey.length(), paraList);
	paraLen += setKey.length() + 20;
	fillCommandPara(member.c_str(), member.size(), paraList);
	paraLen += member.size() + 20;
	bool success = doTransactionCommandInConnection(paraLen,paraList,RedisCommandType::REDIS_COMMAND_SADD, con);
	freeCommandList(paraList);
	if(!success)
	{
//		m_logger.error("sadd of transaction failed");
		LOG(ERROR)<<"sadd of transaction failed";
	}
	return success;
}

bool RedisClient::Srem(RedisConnection* con, const string & setKey, const string& member)
{
	list<RedisCmdParaInfo> paraList;
	int32_t paraLen = 0;
	fillCommandPara("srem", 4, paraList);
	paraLen += 15;
	fillCommandPara(setKey.c_str(), setKey.length(), paraList);
	paraLen += setKey.length() + 20;
	fillCommandPara(member.c_str(), member.size(), paraList);
	paraLen += member.size() + 20;
	bool success = doTransactionCommandInConnection(paraLen,paraList,RedisCommandType::REDIS_COMMAND_SREM, con);
	freeCommandList(paraList);
	if(!success)
	{
//		m_logger.error("srem of transaction failed");
		LOG(ERROR)<<"srem of transaction failed";
	}
	return success;
}



