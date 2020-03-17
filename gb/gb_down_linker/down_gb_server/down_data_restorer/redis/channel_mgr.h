#ifndef DOWN_DATA_RESTORER_CHANNEL_MGR_H
#define DOWN_DATA_RESTORER_CHANNEL_MGR_H

#include"channel.h"
#include"rwmutex.h"
#include"redisbase.h"
#include<list>
#include<string>


namespace GBGateway {

class RedisClient;


// channel key: {downlinker.channel}:gbdownlinker_device_id:device_id:channel_device_id

class ChannelMgr{
public:
	ChannelMgr(RedisClient*);
	
	int GetChannelKeyList(const std::string& gbdownlinker_device_id, std::list<std::string>& channel_key_list);
	int GetChannelKeyListByDeviceId(const std::string& gbdownlinker_device_id, const std::string& device_id, std::list<std::string>& channel_key_list);

	int DeleteChannel(const std::string& channel_key);

	int LoadChannel(const std::list<std::string>& channel_key_list, std::list<ChannelPtr>* channels);
	int LoadChannel(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels);

	int InsertChannel(const std::string& gbdownlinker_device_id, const Channel& channel);
	int UpdateChannel(const std::string& gbdownlinker_device_id, const Channel& channels);
	int DeleteChannel(const std::string& gbdownlinker_device_id, const std::string& device_id);
	int DeleteChannel(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& channel_device_id);
	int ClearChannel(const std::string& gbdownlinker_device_id);

	size_t GetChannelCount(const std::string& gbdownlinker_device_id);

private:
	int InsertChannelInTransaction(const Channel& channel);
	int InsertChannelInCluster(const Channel& channel);
	int DeleteChannelInTransaction(const Channel& channel);
	int DeleteChannelInCluster(const Channel& channel);
	int ClearChannelWithLockHeld();

	RedisClient* redis_client_;
//	RWMutex rwmutex_;
};

} // namespace GBGateway

#endif
