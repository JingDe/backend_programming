#include"channelmgr.h"
#include"redisclient.h"
#include"mutexlockguard.h"
#include"glog/logging.h"

const string ChannelMgr::s_set_key="gbddrchannelset";
const string ChannelMgr::s_key_prefix="gbddrchannelset";

ChannelMgr::ChannelMgr(RedisClient* redis_client, RedisMode redis_mode)
	:redis_client_(redis_client),
	redis_mode_(redis_mode),
	rwmutex_()
//	modify_mutex_()
{
	LOG(INFO)<<"ChannelMgr constructed";
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

	if(redis_client_->setSerial(s_key_prefix+channel.GetChannelId(), channel)==false)
	{
		return -1;
	}
	
	if(redis_client_->sadd(s_set_key, channel.GetChannelId())==true)
	{
		return 0;
	}
	else
	{
		if(redis_client_->del(s_key_prefix+channel.GetChannelId())==false)
		{
			LOG(ERROR)<<"insert channel"<<channel.GetChannelId()<<" failed, undo set failed";
		}
		return -1;
	}
}

int ChannelMgr::InsertChannelInTransaction(const Channel& channel)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);
	
    RedisConnection *con=NULL;
    if(!redis_client_->PrepareTransaction(&con))
    {
    	LOG(ERROR)<<"PrepareTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
	    LOG(ERROR)<<"StartTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->Set(con, s_key_prefix+channel.GetChannelId(), channel))
    {
	    LOG(ERROR)<<"Set failed";
    	goto FAIL;
    }
    if(!redis_client_->Sadd(con, s_set_key, channel.GetChannelId()))
    {
	    LOG(ERROR)<<"Sadd failed";
    	goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
	    LOG(ERROR)<<"ExecTransaction failed";
    	goto FAIL;
    }
            
    redis_client_->FinishTransaction(&con);
    LOG(INFO)<<"InsertChannel success: "<<channel.GetChannelId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
    LOG(ERROR)<<"InsertChannel failed: "<<channel.GetChannelId();
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

	if(redis_client_->del(s_key_prefix+channel.GetChannelId())==false)
		return -1;
	
	if(redis_client_->srem(s_set_key, channel.GetChannelId())==true)
	{
		LOG(INFO)<<"delete device "<<channel.GetChannelId()<<" ok";
		return 0;
	}
	else
	{
		if(redis_client_->setSerial(s_key_prefix+channel.GetChannelId(), channel)==false)
		{
			LOG(ERROR)<<"delete channel"<<channel.GetChannelId()<<" failed, undo del failed";
		}
		return -1;
	}
}

int ChannelMgr::DeleteChannelInTransaction(const Channel& channel)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	RedisConnection *con=NULL;
    if(!redis_client_->PrepareTransaction(&con))
    {
    	LOG(ERROR)<<"PrepareTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
	    LOG(ERROR)<<"StartTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->Del(con, s_key_prefix+channel.GetChannelId()))
    {
	    LOG(ERROR)<<"Del failed";
    	goto FAIL;
    }
    if(!redis_client_->Srem(con, s_set_key, channel.GetChannelId()))
    {
	    LOG(ERROR)<<"Srem failed";
    	goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
	    LOG(ERROR)<<"ExecTransaction failed";
    	goto FAIL;
    }
            
    redis_client_->FinishTransaction(&con);
    LOG(INFO)<<"DeleteChannel success: "<<channel.GetChannelId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
    LOG(ERROR)<<"DeleteChannel failed: "<<channel.GetChannelId();
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
		redis_client_->sadd(s_set_key, it->GetChannelId());
		redis_client_->setSerial(s_key_prefix+it->GetChannelId(), *it);
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
		redis_client_->sadd(s_set_key, channel->GetChannelId());
		redis_client_->setSerial(s_key_prefix+channel->GetChannelId(), *channel);
	}
    return 0;
}

int ChannelMgr::GetChannelCount()
{
	ReadGuard guard(rwmutex_);
	return redis_client_->scard(s_set_key);
}

