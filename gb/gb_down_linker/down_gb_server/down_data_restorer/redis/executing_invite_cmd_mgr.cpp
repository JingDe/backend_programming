#include"executing_invite_cmd_mgr.h"
#include"redisclient.h"
//#include"downdatarestorercommon.h"
#include"redis_client_util.h"
#include"mutexlock.h"
#include"mutexlockguard.h"
#include"serialize_entity.h"
#include"base_library/log.h"

#include<sstream>


namespace GBGateway {


ExecutingInviteCmdMgr::ExecutingInviteCmdMgr(RedisClient* redis_client)
	:redis_client_(redis_client)
{
	LOG_WRITE_INFO("ExecutingInviteCmdMgr constructed");
}

ExecutingInviteCmdMgr::~ExecutingInviteCmdMgr()
{
}

int ExecutingInviteCmdMgr::GetInviteKeyList(const std::string& gbdownlinker_device_id, std::list<std::string>& invite_key_list)
{
	invite_key_list.clear();
	std::string key_prefix = std::string("{downlinker.invite}:") + gbdownlinker_device_id;
	redis_client_->getKeys(key_prefix, invite_key_list);
	return 0;
}

int ExecutingInviteCmdMgr::GetInviteKeyList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<std::string>& invite_key_list)
{
	invite_key_list.clear();
	std::string key_prefix = std::string("{downlinker.invite}:") + gbdownlinker_device_id + std::string(":") + std::to_string(worker_thread_idx);
	redis_client_->getKeys(key_prefix, invite_key_list);
	return 0;
}

int ExecutingInviteCmdMgr::LoadInvite(const std::list<std::string>& invite_key_list, std::list<ExecutingInviteCmdPtr>* invites)
{
	invites->clear();
	for (std::list<std::string>::const_iterator cit = invite_key_list.cbegin(); cit != invite_key_list.cend(); cit++)
	{
		ExecutingInviteCmd* invite = new ExecutingInviteCmd();
		if (!redis_client_->getSerial(*cit, *invite))
			continue;
		//invite->sipRestarted = true;

		ExecutingInviteCmdPtr invite_ptr(invite);
		invites->push_back(std::move(invite_ptr));
	}
	return 0;
}

int ExecutingInviteCmdMgr::LoadInvite(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* invites)
{
	std::list<std::string> invite_key_list;
	GetInviteKeyList(gbdownlinker_device_id, worker_thread_idx, invite_key_list);
	LoadInvite(invite_key_list, invites);
	return 0;
}

int ExecutingInviteCmdMgr::Insert(const std::string& gbdownlinker_device_id, int worker_thread_idx, const ExecutingInviteCmd& cmd)
{
	std::string invite_key = std::string("{downlinker.invite}:") + gbdownlinker_device_id + ":" + std::to_string(worker_thread_idx)+":"+cmd.cmdSenderId +":"+cmd.deviceId +":"+std::to_string(cmd.cmdSeq);
	if(redis_client_->setSerial(invite_key, cmd)==false)
	{
		return -1;
	}
	
	return 0;
}

int ExecutingInviteCmdMgr::Update(const std::string& gbdownlinker_device_id, int worker_thread_idx, const ExecutingInviteCmd& cmd)
{
	return Insert(gbdownlinker_device_id, worker_thread_idx, cmd);
}

int ExecutingInviteCmdMgr::DeleteInvite(const std::string& invite_key)
{
	if (redis_client_->del(invite_key) == false)
		return -1;

	return 0;
}

int ExecutingInviteCmdMgr::Delete(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq)
{
	std::string invite_key = std::string("{downlinker.invite}:") + gbdownlinker_device_id + ":" + std::to_string(worker_thread_idx) + ":" + cmd_sender_id + ":" + device_id + ":" + std::to_string(cmd_seq);
	if(redis_client_->del(invite_key)==false)
		return -1;
	
	return 0;
}

int ExecutingInviteCmdMgr::Clear(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	std::list<std::string> invite_key_list;
	GetInviteKeyList(gbdownlinker_device_id, worker_thread_idx, invite_key_list);

	for (std::list<std::string>::iterator it = invite_key_list.begin(); it != invite_key_list.end(); it++)
	{
		redis_client_->del(*it);
	}
	return 0;
}

size_t ExecutingInviteCmdMgr::GetInviteCount(const std::string& gbdownlinker_device_id)
{
	std::list<std::string> invite_key_list;
	GetInviteKeyList(gbdownlinker_device_id, invite_key_list);
	return invite_key_list.size();
}

size_t ExecutingInviteCmdMgr::GetInviteCount(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	std::list<std::string> invite_key_list;
	GetInviteKeyList(gbdownlinker_device_id, worker_thread_idx, invite_key_list);
	return invite_key_list.size();
}

} // namespace GBGateway
