#ifndef DOWN_DATA_RESTORER_EXECUTING_INVITE_CMD_H
#define DOWN_DATA_RESTORER_EXECUTING_INVITE_CMD_H

#include "executing_invite_cmd.h"
#include "down_data_restorer_def.h"
#include "redisbase.h"

#include <memory>
#include <list>


namespace GBDownLinker {

class RedisClient;
class MutexLock;

// {downlinker.invite}:gbdownlinker_device_id:worker_thread_idx:cmd_sender_id:device_id:cmd_seq

class ExecutingInviteCmdMgr {
public:
	ExecutingInviteCmdMgr(RedisClient* redis_client);
	~ExecutingInviteCmdMgr();

	int GetInviteKeyList(const std::string& gbdownlinker_device_id, std::list<std::string>& invite_key_list);
	int GetInviteKeyList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<std::string>& invite_key_list);

	int DeleteInvite(const std::string& invite_key);

	int LoadInvite(const std::list<std::string>& invite_key_list, std::list<ExecutingInviteCmdPtr>* invites);
	int LoadInvite(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* invites);
	int InsertInvite(const std::string& gbdownlinker_device_id, int worker_thread_no, const ExecutingInviteCmd& cmd);
	int UpdateInvite(const std::string& gbdownlinker_device_id, int worker_thread_no, const ExecutingInviteCmd& cmd);
	int DeleteInvite(const std::string& gbdownlinker_device_id, int worker_thread_no, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq);
	int ClearInvite(const std::string& gbdownlinker_device_id, int worker_thread_no);

	size_t GetInviteCount(const std::string& gbdownlinker_device_id);
	size_t GetInviteCount(const std::string& gbdownlinker_device_id, int worker_thread_idx);

private:

	RedisClient* redisClient_;
};

} // namespace GBDownLinker

#endif
