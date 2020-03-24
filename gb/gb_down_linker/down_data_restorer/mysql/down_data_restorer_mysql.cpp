#include "down_data_restorer_mysql.h"
#include "down_data_restorer_def.h"

namespace GBDownLinker {

CDownDataRestorerMySql::~CDownDataRestorerMySql()
{
}

int CDownDataRestorerMySql::Init(const RestorerParamPtr& restorer_param)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::Uninit()
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::Start()
{
	return RESTORER_OK;
}
int CDownDataRestorerMySql::Stop()
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::GetStats()
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::DeleteDeviceList(const std::string& gbdownlinker_device_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::DeleteChannelList(const std::string& gbdownlinker_device_id)
{
	return RESTORER_OK;
}


int CDownDataRestorerMySql::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& device_channel_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq)
{
	return RESTORER_OK;
}

int CDownDataRestorerMySql::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	return RESTORER_OK;
}

} // namespace GBDownLinker
