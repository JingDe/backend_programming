#include"channel_mgr.h"
#include"redisclient.h"
#include"mutexlockguard.h"
#include "base_library/log.h"
#include"serialize_entity.h"
#include<sstream>

namespace GBGateway {

const string ChannelMgr::s_set_key="gbddrchannelset";
const string ChannelMgr::s_key_prefix="gbddrchannelset";

ChannelMgr::ChannelMgr(RedisClient* redis_client, RedisMode redis_mode)
	:redis_client_(redis_client),
	redis_mode_(redis_mode),
	rwmutex_()
//	modify_mutex_()
{
	LOG_WRITE_INFO("ChannelMgr constructed");
}

int ChannelMgr::LoadChannels(list<Channel>& channels)
{
	ReadGuard guard(rwmutex_);
	list<string> channel_id_list;
	redis_client_->smembers(s_set_key, channel_id_list);
	for(list<string>::iterator it=channel_id_list.begin(); it!=channel_id_list.end(); it++)
	{
		Channel channel;
		if(redis_client_->getSerial(s_key_prefix+*it, channel))
		{
			channels.push_back(channel);
		}
	}
	return 0;
}

int ChannelMgr::SearchChannel(const string& channel_id, Channel& channel)
{
	ReadGuard guard(rwmutex_);
	bool exist=redis_client_->sismember(s_set_key, channel_id);
	if(exist)
	{
		if(redis_client_->getSerial(s_key_prefix+channel_id, channel))
		{
			return 0;
		}
	}	
	return -1;
}

int ChannelMgr::InsertChannel(const Channel& channel)
{
	if(redis_mode_==STAND_ALONE_OR_PROXY_MODE)
		return InsertChannelInTransaction(channel);
	else //if(redis_mode_==CLUSTER_MODE)
		return InsertChannelInCluster(channel);
}


int ChannelMgr::InsertChannelInCluster(const Channel& channel)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	if(redis_client_->setSerial(s_key_prefix+(channel.deviceId+channel.channelDeviceId), channel)==false)
	{
		return -1;
	}
	
	if(redis_client_->sadd(s_set_key, (channel.deviceId+channel.channelDeviceId))==true)
	{
		return 0;
	}
	else
	{
		if(redis_client_->del(s_key_prefix+(channel.deviceId+channel.channelDeviceId))==false)
		{
			std::stringstream log_msg;
			log_msg<<"insert channel"<<(channel.deviceId+channel.channelDeviceId)<<" failed, undo set failed";
			LOG_WRITE_ERROR(log_msg.str());
		}
		return -1;
	}
}

int ChannelMgr::InsertChannelInTransaction(const Channel& channel)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);
	
    std::stringstream log_msg;
    RedisConnection *con=NULL;
    if(!redis_client_->PrepareTransaction(&con))
    {
    	goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
    	goto FAIL;
    }
    if(!redis_client_->Set(con, s_key_prefix+(channel.deviceId+channel.channelDeviceId), channel))
    {
    	goto FAIL;
    }
    if(!redis_client_->Sadd(con, s_set_key, (channel.deviceId+channel.channelDeviceId)))
    {
    	goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
    	goto FAIL;
    }
            
    redis_client_->FinishTransaction(&con);
	log_msg.str("");
	log_msg<<"InsertChannel success: "<<(channel.deviceId+channel.channelDeviceId);
	LOG_WRITE_INFO(log_msg.str());
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
	log_msg.str("");
	log_msg<<"InsertChannel failed: "<<(channel.deviceId+channel.channelDeviceId);
	LOG_WRITE_ERROR(log_msg.str());
    return -1;
}

int ChannelMgr::DeleteChannel(const Channel& channel)
{
	if(redis_mode_==STAND_ALONE_OR_PROXY_MODE)
		return DeleteChannelInTransaction(channel);
	else //if(redis_mode_==CLUSTER_MODE)
		return DeleteChannelInCluster(channel);
}

int ChannelMgr::DeleteChannelInCluster(const Channel& channel)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	if(redis_client_->del(s_key_prefix+(channel.deviceId+channel.channelDeviceId))==false)
		return -1;
	
	if(redis_client_->srem(s_set_key, (channel.deviceId+channel.channelDeviceId))==true)
	{
		std::stringstream log_msg;
		log_msg<<"delete device "<<(channel.deviceId+channel.channelDeviceId)<<" ok";
		LOG_WRITE_INFO(log_msg.str());
		return 0;
	}
	else
	{
		if(redis_client_->setSerial(s_key_prefix+(channel.deviceId+channel.channelDeviceId), channel)==false)
		{
			std::stringstream log_msg;
			log_msg<<"delete channel"<<(channel.deviceId+channel.channelDeviceId)<<" failed, undo del failed";
			LOG_WRITE_ERROR(log_msg.str());
		}
		return -1;
	}
}

int ChannelMgr::DeleteChannelInTransaction(const Channel& channel)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);
    
	std::stringstream log_msg;

	RedisConnection *con=NULL;
    if(!redis_client_->PrepareTransaction(&con))
    {
    	goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
    	goto FAIL;
    }
    if(!redis_client_->Del(con, s_key_prefix+(channel.deviceId+channel.channelDeviceId)))
    {
    	goto FAIL;
    }
    if(!redis_client_->Srem(con, s_set_key, (channel.deviceId+channel.channelDeviceId)))
    {
    	goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
    	goto FAIL;
    }
            
    redis_client_->FinishTransaction(&con);
	log_msg.str("");
	log_msg<<"DeleteChannel success: "<<(channel.deviceId+channel.channelDeviceId);
	LOG_WRITE_INFO(log_msg.str());
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
	log_msg.str("");
	log_msg<<"DeleteChannel failed: "<<(channel.deviceId+channel.channelDeviceId);
	LOG_WRITE_ERROR(log_msg.str());
    return -1;
}

int ChannelMgr::ClearChannels()
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);
	
	return ClearChannelsWithLockHeld();
}

int ChannelMgr::ClearChannelsWithLockHeld()
{	
	list<string> channel_id_list;
	redis_client_->smembers(s_set_key, channel_id_list);
	redis_client_->del(s_set_key);
	for(list<string>::iterator it=channel_id_list.begin(); it!=channel_id_list.end(); it++)
	{
		redis_client_->del(s_key_prefix+*it);
	}
	return 0;
}

int ChannelMgr::UpdateChannels(const list<Channel>& channels)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	ClearChannelsWithLockHeld();
	
	for(list<Channel>::const_iterator it=channels.begin(); it!=channels.end(); ++it)
	{
		redis_client_->sadd(s_set_key, (it->deviceId+it->channelDeviceId));
		redis_client_->setSerial(s_key_prefix+(it->deviceId+it->channelDeviceId), *it);
	}
    return 0;
}

int ChannelMgr::UpdateChannels(const list<void*>& channels)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	ClearChannelsWithLockHeld();
	
	for(list<void*>::const_iterator it=channels.begin(); it!=channels.end(); ++it)
	{
		Channel* channel=(Channel*)(*it);
		redis_client_->sadd(s_set_key, (channel->deviceId+channel->channelDeviceId));
		redis_client_->setSerial(s_key_prefix+(channel->deviceId+channel->channelDeviceId), *channel);
	}
    return 0;
}

int ChannelMgr::GetChannelCount()
{
	ReadGuard guard(rwmutex_);
	return redis_client_->scard(s_set_key);
}

} // namespace GBGateway
