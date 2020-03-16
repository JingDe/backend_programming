#ifndef DOWN_DATA_RESTORER_EXECUTING_INVITE_CMD_H
#define DOWN_DATA_RESTORER_EXECUTING_INVITE_CMD_H

#include"executing_invite_cmd.h"
#include"rwmutex.h"
#include"down_data_restorer_def.h"
#include"redisbase.h"

#include<memory>
#include<list>


namespace GBGateway {

class RedisClient;
class MutexLock;

// {downlinker.invite}:gbdownlinker_device_id:worker_thread_idx:cmd_sender_id:device_id:cmd_seq

class ExecutingInviteCmdMgr{
public:
	ExecutingInviteCmdMgr(RedisClient* redis_client);
	~ExecutingInviteCmdMgr();
	
	int GetInviteKeyList(const std::string& gbdownlinker_device_id, std::list<std::string>& invite_key_list);
	int GetInviteKeyList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<std::string>& invite_key_list);

	int DeleteInvite(const std::string& invite_key);

	int Load(const std::list<std::string>& invite_key_list, std::list<ExecutingInviteCmdPtr>* invites);
	int Load(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* invites);
	int Insert(const std::string& gbdownlinker_device_id, int worker_thread_no, const ExecutingInviteCmd& cmd);
	int Update(const std::string& gbdownlinker_device_id, int worker_thread_no, const ExecutingInviteCmd& cmd);
	int Delete(const std::string& gbdownlinker_device_id, int worker_thread_no, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq);
	int Clear(const std::string& gbdownlinker_device_id, int worker_thread_no);

	size_t GetDeviceCount(const std::string& gbdownlinker_device_id);
	size_t GetDeviceCount(const std::string& gbdownlinker_device_id, int worker_thread_idx);
	
private:

	RedisClient* redis_client_;
	RWMutex* rwmutexes_;
};

} // namespace GBGateway

#endif
