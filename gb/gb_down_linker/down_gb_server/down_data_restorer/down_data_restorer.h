#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_H

#include<list>
#include<string>

#include"down_data_restorer_def.h"

class Device;
class Channel;
class ExecutingInviteCmd;

namespace GBGateway {

class CDownDataRestorer {
public:
	virtual int Init(RestorerParamPtr restorer_param/*, ErrorReportCallback error_callback*/)=0;
	virtual int Uninit()=0;
    virtual int Start()=0;
    virtual int Stop()=0;
    virtual int GetStat()=0;

	virtual int PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number)=0;

	virtual int LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices)=0;
	virtual int InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device)=0;
	virtual int UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device)=0;
	virtual int DeleteDeviceList(const std::string& gbdownlinker_device_id)=0;
	virtual int DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id)=0;

	virtual int LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channels)=0;
	virtual int InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel)=0;
	virtual int UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel)=0;	
	virtual int DeleteChannelList(const std::string& gbdownlinker_device_id)=0;
	virtual int DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id)=0;
	virtual int DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& device_channel_id)=0;

	virtual int LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)=0;
	virtual int InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)=0;
	virtual int UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd)=0;
	virtual int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const std::string& cmd_seq)=0;
	virtual int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx)=0;
	
};

} // namespace GBGateway

#endif
