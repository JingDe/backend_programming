#include"down_data_restorer_mysql.h"

namespace GBGateway {

CDownDataRestorerMySql::~CDownDataRestorerMySql()
{
}

int CDownDataRestorerMySql::Init(const RestorerParamPtr& restorer_param)
{
	return 0;
}

int CDownDataRestorerMySql::Uninit()
{
	return 0;
}

int CDownDataRestorerMySql::Start()
{
	return 0;
}
int CDownDataRestorerMySql::Stop()
{
	return 0;
}

int CDownDataRestorerMySql::GetStat(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	return 0;
}

int CDownDataRestorerMySql::PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	return 0;
}

int CDownDataRestorerMySql::LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices)
{
	return 0;
}

int CDownDataRestorerMySql::InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return 0;
}

int CDownDataRestorerMySql::UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return 0;
}

int CDownDataRestorerMySql::DeleteDeviceList(const std::string& gbdownlinker_device_id)
{
	return 0;
}

int CDownDataRestorerMySql::DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return 0;
}

int CDownDataRestorerMySql::LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels)
{
	return 0;
}

int CDownDataRestorerMySql::InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return 0;
}

int CDownDataRestorerMySql::UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return 0;
}

int CDownDataRestorerMySql::DeleteChannelList(const std::string& gbdownlinker_device_id)
{
	return 0;
}


int CDownDataRestorerMySql::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return 0;
}

int CDownDataRestorerMySql::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& device_channel_id)
{
	return 0;
}

int CDownDataRestorerMySql::LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)
{
	return 0;
}

int CDownDataRestorerMySql::InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return 0;
}

int CDownDataRestorerMySql::UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return 0;
}

int CDownDataRestorerMySql::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq)
{
	return 0;
}

int CDownDataRestorerMySql::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	return 0;
}
    
} // namespace GBGateway
