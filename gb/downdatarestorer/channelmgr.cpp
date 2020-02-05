#include"channelmgr.h"
#include"redisclient.h"
#include"mutexlockguard.h"
#include"glog/logging.h"

const string ChannelMgr::s_set_key="channelset";
const string ChannelMgr::s_key_prefix="channelset";

ChannelMgr::ChannelMgr(RedisClient* redis_client)
	:redis_client_(redis_client),
	modify_mutex_()
{
	LOG(INFO)<<"ChannelMgr constructed";
}

int ChannelMgr::LoadChannels(list<Channel>& channels)
{
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
	MutexLockGuard guard(&modify_mutex_);
	
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
    if(!redis_client_->Set(con, s_key_prefix+channel.GetId(), channel))
    {
	    LOG(ERROR)<<"Set failed";
    	goto FAIL;
    }
    if(!redis_client_->Sadd(con, s_set_key, channel.GetId()))
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
    LOG(INFO)<<"InsertChannel success: "<<channel.GetId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
    LOG(ERROR)<<"InsertChannel failed: "<<channel.GetId();
    return -1;
}

int ChannelMgr::DeleteChannel(const Channel& channel)
{
	MutexLockGuard guard(&modify_mutex_);

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
    if(!redis_client_->Del(con, s_key_prefix+channel.GetId()))
    {
	    LOG(ERROR)<<"Del failed";
    	goto FAIL;
    }
    if(!redis_client_->Srem(con, s_set_key, channel.GetId()))
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
    LOG(INFO)<<"DeleteChannel success: "<<channel.GetId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
    LOG(ERROR)<<"DeleteChannel failed: "<<channel.GetId();
    return -1;
}

int ChannelMgr::ClearChannels()
{
	MutexLockGuard guard(&modify_mutex_);
	
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
	MutexLockGuard guard(&modify_mutex_);
	
	for(list<Channel>::const_iterator it=channels.begin(); it!=channels.end(); ++it)
	{
		redis_client_->sadd(s_set_key, it->GetId());
		redis_client_->setSerial(s_key_prefix+it->GetId(), *it);
	}
    return 0;
}

int ChannelMgr::UpdateChannels(const list<void*>& channels)
{
	MutexLockGuard guard(&modify_mutex_);
	
	for(list<void*>::const_iterator it=channels.begin(); it!=channels.end(); ++it)
	{
		Channel* channel=(Channel*)(*it);
		redis_client_->sadd(s_set_key, channel->GetId());
		redis_client_->setSerial(s_key_prefix+channel->GetId(), *channel);
	}
    return 0;
}

int ChannelMgr::GetChannelCount()
{
	return redis_client_->scard(s_set_key);
}

