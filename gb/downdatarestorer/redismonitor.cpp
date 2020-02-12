#include "redismonitor.h"
#include"glog/logging.h"

RedisMonitor::RedisMonitor(RedisClient* redis_client)
	:m_redisClient(redis_client)
{	
	LOG(INFO)<<"construct RedisMonitor ok";
}

RedisMonitor::~RedisMonitor()
{
}

void RedisMonitor::run()
{
	if (m_redisClient == NULL)
	{
//		m_logger.warn("redis client is NULL, can do nothing.");
		return;
	}
	else if(m_redisClient->isRunAsCluster()==false)
	{
//		m_logger.warn("redis is proxy mode, do nothing");
		return ;
	}
	//
	m_redisClient->releaseUnusedClusterHandler();
	//
	REDIS_CLUSTER_MAP clusterMap;
	if (!m_redisClient->getRedisClustersByCommand(clusterMap))
	{
//		m_logger.warn("get redis clusters by command failed.");
		return;
	}
	REDIS_CLUSTER_MAP oldClusterMap;
	m_redisClient->getRedisClusters(oldClusterMap);
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
		//
	}
	if (change)
	{
		m_redisClient->checkAndSaveRedisClusters(clusterMap);
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
		m_redisClient->checkAndSaveRedisClusters(clusterMap);
		return;
	}
//	m_logger.info("no change do noting");
	return;
}

