#include"channel_mgr.h"
#include"redisclient.h"
#include"mutexlockguard.h"
#include"base_library/log.h"
#include"serialize_entity.h"
#include<sstream>

namespace GBGateway {


ChannelMgr::ChannelMgr(RedisClient* redis_client)
	:redis_client_(redis_client),
	rwmutex_()
{
	LOG_WRITE_INFO("ChannelMgr constructed");
}

int ChannelMgr::GetChannelKeyList(std::string gbdownlinker_device_id, std::list<std::string>& channel_key_list)
{
	channel_key_list.clear();
	std::string key_prefix = std::string("{downlinker.channel}:") + gbdownlinker_device_id;
	redis_client_->getKeys(key_prefix, channel_key_list);
	return 0;
}

int ChannelMgr::LoadChannel(const std::list<std::string>& channel_key_list, std::list<ChannelPtr>* channels)
{
	channels.clear();
	for (std::list<std::string>::const_iterator cit = channel_key_list.cbegin(); cit != channel_key_list.cend(); cit++)
	{
		Channel* channel=new Channel();
		if (!redis_client_->getSerial(*it, *channel))
			continue;
		channel->registerLastTime = base::CTime::GetCurrentTime();
		channel->keepaliveLastTime = base::CTime::GetCurrentTime();

		ChannelPtr channel_ptr(channel);
		channels.push_back(channel_ptr);
	}
	return 0;
}

int ChannelMgr::LoadChannel(std::string gbdownlinker_device_id, std::list<ChannelPtr>* channels)
{
	std::list<std::string> channel_key_list;
	GetChannelKeyList(gbdownlinker_device_id, channel_key_list);
	LoadChannels(channel_key_list, channels);
	return 0;
}


int ChannelMgr::InsertChannel(const std::string& gbdownlinker_device_id, const Channel& channel)
{
	WriteGuard guard(rwmutex_);

	std::string channel_key = std::string("{downlinker.channel}:") + gbdownlinker_device_id + ":" + channel.channelId;
	if(redis_client_->setSerial(channel_key, channel)==false)
	{
		return -1;
	}
	return 0;
}

int ChannelMgr::InsertChannel(const std::string& gbdownlinker_device_id, const Channel& channel)
{
	return InsertChannel(gbdownlinker_device_id, channel);
}

int ChannelMgr::DeleteChannel(const std::string& channel_key)
{
	WriteGuard guard(rwmutex_);

	if (redis_client_->del(channel_key) == false)
		return -1;

	return 0;
}

int ChannelMgr::DeleteChannel(const std::string& gbdownlinker_device_id, const std::string& channel_id)
{
	WriteGuard guard(rwmutex_);

	std::string channel_key = std::string("{downlinker.channel}:") + gbdownlinker_device_id + ":" + channel_id;
	if(redis_client_->del(channel_key)==false)
		return -1;
	
	return 0;
}

int ChannelMgr::ClearChannel(const std::string& gbdownlinker_device_id)
{
	WriteGuard guard(rwmutex_);

	std::list<std::string> channel_key_list;
	GetChannelKeyList(gbdownlinker_device_id, channel_key_list);

	for(std::list<std::string>::iterator it= channel_key_list.begin(); it!= channel_key_list.end(); it++)
	{
		redis_client_->del(*it);
	}
	return 0;
}

int ChannelMgr::GetChannelCount(const std::string& gbdownlinker_device_id)
{
	ReadGuard guard(rwmutex_);
	std::list<std::string> channel_key_list;
	GetChannelKeyList(gbdownlinker_device_id, channel_key_list);
	return channel_key_list.size();
}

} // namespace GBGateway
