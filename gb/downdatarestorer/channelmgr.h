#ifndef DOWN_DATA_RESTORER_CHANNEL_MGR_H
#define DOWN_DATA_RESTORER_CHANNEL_MGR_H

#include"channel.h"
//#include"mutexlock.h"
#include"rwmutex.h"
#include<list>
#include<string>

using std::list;
using std::string;

class RedisClient;

class ChannelMgr{
public:
	ChannelMgr(RedisClient*);

	int LoadChannels(list<Channel>& channels);
	int SearchChannel(const string& channel_id, Channel& channel);
	int InsertChannel(const Channel& channel);
	int DeleteChannel(const Channel& channel);
	int ClearChannels();
	int UpdateChannels(const list<Channel>& channels);
	int UpdateChannels(const list<void*>& channels);
	int GetChannelCount();

private:
	RedisClient* redis_client_;
//	MutexLock modify_mutex_;
	RWMutex rwmutex_;

	const static string s_set_key;
	const static string s_key_prefix;
};

#endif
