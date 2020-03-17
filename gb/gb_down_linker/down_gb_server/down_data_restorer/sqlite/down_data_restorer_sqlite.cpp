#include"down_data_restorer_sqlite.h"

namespace GBGateway {

CDownDataRestorerSqlite::~CDownDataRestorerSqlite()
{
}

int CDownDataRestorerSqlite::Init(const RestorerParamPtr& restorer_param)
{
	return 0;
}

int CDownDataRestorerSqlite::Uninit()
{
	return 0;
}

int CDownDataRestorerSqlite::Start()
{
	return 0;
}
int CDownDataRestorerSqlite::Stop()
{
	return 0;
}

int CDownDataRestorerSqlite::GetStat(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	return 0;
}

int CDownDataRestorerSqlite::PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	return 0;
}

int CDownDataRestorerSqlite::LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices)
{
	return 0;
}

int CDownDataRestorerSqlite::InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return 0;
}

int CDownDataRestorerSqlite::UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return 0;
}

int CDownDataRestorerSqlite::DeleteDeviceList(const std::string& gbdownlinker_device_id)
{
	return 0;
}

int CDownDataRestorerSqlite::DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return 0;
}

int CDownDataRestorerSqlite::LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels)
{
	return 0;
}

int CDownDataRestorerSqlite::InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return 0;
}

int CDownDataRestorerSqlite::UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return 0;
}

int CDownDataRestorerSqlite::DeleteChannelList(const std::string& gbdownlinker_device_id)
{
	return 0;
}


int CDownDataRestorerSqlite::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return 0;
}

int CDownDataRestorerSqlite::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& device_channel_id)
{
	return 0;
}

int CDownDataRestorerSqlite::LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)
{
	return 0;
}

int CDownDataRestorerSqlite::InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return 0;
}

int CDownDataRestorerSqlite::UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return 0;
}

int CDownDataRestorerSqlite::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq)
{
	return 0;
}

int CDownDataRestorerSqlite::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	return 0;
}
    
} // namespace GBGateway
