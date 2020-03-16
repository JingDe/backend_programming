#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_SQLITE_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_SQLITE_H

#include"down_data_restorer.h"

namespace GBGateway {

class CDownDataRestorerSqlite : public CDownDataRestorer {
public:
	int Init(const RestorerParamPtr& restorer_param);
    int Uninit();
    int Start();
    int Stop();
    int GetStat(const std::string& gbdownlinker_device_id, int worker_thread_number);

    int PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number);

    int LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices);
    int InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device);
    int UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device);
    int DeleteDeviceList(const std::string& gbdownlinker_device_id);
    int DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id);

    int LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels);
    int InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel);
    int UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel); 
    int DeleteChannelList(const std::string& gbdownlinker_device_id);
    int DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id);
    int DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& device_channel_id);

	int LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists);
    int InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd);
    int UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd);
    int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq);
    int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx);
    
};

} // namespace GBGateway

#endif
