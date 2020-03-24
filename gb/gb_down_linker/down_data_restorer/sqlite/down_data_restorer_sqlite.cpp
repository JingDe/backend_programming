#include "down_data_restorer_sqlite.h"
#include "down_data_restorer_def.h"

namespace GBDownLinker {

CDownDataRestorerSqlite::~CDownDataRestorerSqlite()
{
}

int CDownDataRestorerSqlite::Init(const RestorerParamPtr& restorer_param)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::Uninit()
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::Start()
{
	return RESTORER_OK;
}
int CDownDataRestorerSqlite::Stop()
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::GetStats()
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::DeleteDeviceList(const std::string& gbdownlinker_device_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::DeleteChannelList(const std::string& gbdownlinker_device_id)
{
	return RESTORER_OK;
}


int CDownDataRestorerSqlite::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& device_channel_id)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq)
{
	return RESTORER_OK;
}

int CDownDataRestorerSqlite::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	return RESTORER_OK;
}

} // namespace GBDownLinker
