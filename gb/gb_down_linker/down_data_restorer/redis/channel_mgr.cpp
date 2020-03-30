#include "channel_mgr.h"
#include "redisclient.h"
#include "mutexlockguard.h"
#include "base_library/log.h"
#include "serialize_entity.h"
#include <sstream>

namespace GBDownLinker {


ChannelMgr::ChannelMgr(RedisClient* redis_client)
	:redisClient_(redis_client)
{
	LOG_WRITE_INFO("ChannelMgr constructed");
}

int ChannelMgr::GetChannelKeyList(const std::string& gbdownlinker_device_id, std::list<std::string>& channel_key_list)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	channel_key_list.clear();
	std::string key_prefix = std::string("{downlinker.channel}:") + gbdownlinker_device_id;
	redisClient_->getKeys(key_prefix, channel_key_list);
	return 0;
}

int ChannelMgr::GetChannelKeyListByDeviceId(const std::string& gbdownlinker_device_id, const std::string& device_id, std::list<std::string>& channel_key_list)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);
	
	channel_key_list.clear();
	std::string key_prefix = std::string("{downlinker.channel}:") + gbdownlinker_device_id + ":" + device_id;
	redisClient_->getKeys(key_prefix, channel_key_list);
	return 0;
}

int ChannelMgr::LoadChannel(const std::list<std::string>& channel_key_list, std::list<ChannelPtr>* channels)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	channels->clear();
	for (std::list<std::string>::const_iterator cit = channel_key_list.cbegin(); cit != channel_key_list.cend(); cit++)
	{
		Channel* channel = new Channel();
		if (!redisClient_->getSerial(*cit, *channel))
			continue;

		ChannelPtr channel_ptr(channel);
		channels->push_back(std::move(channel_ptr));
	}
	return 0;
}

int ChannelMgr::LoadChannel(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	std::list<std::string> channel_key_list;
	GetChannelKeyList(gbdownlinker_device_id, channel_key_list);
	LoadChannel(channel_key_list, channels);
	return 0;
}


int ChannelMgr::InsertChannel(const std::string& gbdownlinker_device_id, const Channel& channel)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	std::string channel_key = std::string("{downlinker.channel}:") + gbdownlinker_device_id + ":" + channel.deviceId + ":" + channel.channelDeviceId;
	if (redisClient_->setSerial(channel_key, channel) == false)
	{
		return -1;
	}
	return 0;
}

int ChannelMgr::UpdateChannel(const std::string& gbdownlinker_device_id, const Channel& channel)
{
	return InsertChannel(gbdownlinker_device_id, channel);
}

int ChannelMgr::DeleteChannel(const std::string& channel_key)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);
	
	if (redisClient_->del(channel_key) == false)
		return -1;

	return 0;
}

int ChannelMgr::DeleteChannel(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);
	
	std::list<std::string> channel_key_list;
	GetChannelKeyListByDeviceId(gbdownlinker_device_id, device_id, channel_key_list);

	for (std::list<std::string>::iterator it = channel_key_list.begin(); it != channel_key_list.end(); ++it)
	{
		redisClient_->del(*it);
	}
	return 0;
}

int ChannelMgr::DeleteChannel(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& channel_device_id)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);
	
	std::string channel_key = std::string("{downlinker.channel}:") + gbdownlinker_device_id + ":" + device_id + ":" + channel_device_id;
	if (redisClient_->del(channel_key) == false)
		return -1;

	return 0;
}

int ChannelMgr::ClearChannel(const std::string& gbdownlinker_device_id)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);
	
	std::list<std::string> channel_key_list;
	GetChannelKeyList(gbdownlinker_device_id, channel_key_list);

	for (std::list<std::string>::iterator it = channel_key_list.begin(); it != channel_key_list.end(); it++)
	{
		redisClient_->del(*it);
	}
	return 0;
}

size_t ChannelMgr::GetChannelCount(const std::string& gbdownlinker_device_id)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);
	
	std::list<std::string> channel_key_list;
	GetChannelKeyList(gbdownlinker_device_id, channel_key_list);
	return channel_key_list.size();
}

} // namespace GBDownLinker
