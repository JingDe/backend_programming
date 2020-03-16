#include"serialize_entity.h"

namespace GBGateway {


template<>
void save(const Channel& channel, DBOutStream& out)
{
	out<<channel.deviceId;
	out<<channel.channelDeviceId;
	out<<channel.channelName;
    out<<channel.channelManufacturer;
    out<<channel.channelModel;
    out<<channel.channelOwner;
    out<<channel.channelCivilCode;
    out<<channel.channelAddress;
    out<<channel.channelParental;
    out<<channel.channelParentId;
    out<<channel.channelSafetyWay;
    out<<channel.channelRegisterWay;
    out<<channel.channelSecrecy;
	out<<static_cast<uint8_t>(channel.channelStatus);
}

template<>
void load(DBInStream& in, Channel& channel)
{
	in>>channel.deviceId;
	in>>channel.channelDeviceId;
	in>>channel.channelName;
    in>>channel.channelManufacturer;
    in>>channel.channelModel;
    in>>channel.channelOwner;
    in>>channel.channelCivilCode;
    in>>channel.channelAddress;
    in>>channel.channelParental;
    in>>channel.channelParentId;
    in>>channel.channelSafetyWay;
    in>>channel.channelRegisterWay;
    in>>channel.channelSecrecy;
	uint8_t status;
	in>>status;
	channel.channelStatus=static_cast<ChannelStatus>(status);
}

void savetime(const TimePtr& timeptr, DBOutStream& out)
{
	if(timeptr==nullptr)
	{
		out<<static_cast<uint8_t>(0);
		return;
	}
	else
	{
		out<<static_cast<uint8_t>(1);
		
		struct tm* tmptr=timeptr->GetTmPtr();
		out<<tmptr->tm_sec;
		out<<tmptr->tm_min;
		out<<tmptr->tm_hour;
		out<<tmptr->tm_mday;
		out<<tmptr->tm_mon;
		out<<tmptr->tm_year;
		out<<tmptr->tm_wday;
		out<<tmptr->tm_yday;
		out<<tmptr->tm_isdst;
	}
}

void loadtime(DBInStream& in, TimePtr& timeptr)
{
	uint8_t flag;
	in>>flag;
	
	if(flag==uint8_t(0))
	{
		timeptr=nullptr;
		return;
	}
	else
	{	
		struct tm* time=(struct tm*)malloc(sizeof(struct tm));
		in>>time->tm_sec;
		in>>time->tm_min;
		in>>time->tm_hour;
		in>>time->tm_mday;
		in>>time->tm_mon;
		in>>time->tm_year;
		in>>time->tm_wday;
		in>>time->tm_yday;
		in>>time->tm_isdst;
		
		timeptr=std::make_unique<base::CTime>(time);
	}
}

void savechannellist(const ChannelListPtr& channellist, DBOutStream& out)
{
	ChannelListPtr channel_list=channellist->Clone();
	std::list<ChannelPtr>& channelptr_list=channel_list->GetChannelList();
	
	//out<<static_cast<uint32_t>(channel_list->size());
	out<<static_cast<uint32_t>(channel_list->GetChannelCount());	

	for(ChannelPtr& channel_ptr : channelptr_list)
	{
		out<<channel_ptr->deviceId;
		out<<channel_ptr->channelDeviceId;
	}
}

void loadchannellist(DBInStream& in, ChannelListPtr& channellist)
{
	channellist=std::make_unique<CChannelList>();
	
	uint32_t channel_size;
	in>>channel_size;
	for(uint32_t i=0; i<channel_size; i++)
	{
		string deviceId;
		string channelDeviceId;
		in>>deviceId;
		in>>channelDeviceId;
		
		ChannelPtr channel_ptr=std::make_unique<Channel>();
		if(channel_ptr==nullptr)
			continue;
		channel_ptr->deviceId=deviceId;
		channel_ptr->channelDeviceId=channelDeviceId;
		channellist->GetChannelList().push_back(std::move(channel_ptr));
	}
}

template<>
void save(const Device& device, DBOutStream& out)
{
	out<<device.deviceId;
    out<<device.deviceSipIp;
    out<<device.deviceSipPort;
    out<<static_cast<uint8_t>(device.deviceState);
  
	savetime(device.deviceStateChangeTime, out);
	savetime(device.registerLastTime, out);
    
	out<<device.registerExpires;
    
	savetime(device.keepaliveLastTime, out);
    
	out<<device.keepaliveExpires;
	out<<static_cast<uint8_t>(device.queryCatalogState);
    out<<device.queryCatalogSN;
    out<<device.queryCatalogNumber;
    out<<device.queryCatalogSipTid;
	out<<static_cast<uint8_t>(device.subscribeCatalogState);
    out<<device.subscribeCatalogSN;
    out<<device.subscribeCatalogNumber;
    out<<device.subscribeCatalogSipSid;
    out<<device.keepaliveTimerId;
                                                                                                                 
	savechannellist(device.channelList, out);
}

template<>
void load(DBInStream& in, Device& device)
{
	in>>device.deviceId;
    in>>device.deviceSipIp;
    in>>device.deviceSipPort;
	uint8_t state;
	in>>state;
	device.deviceState=static_cast<DeviceState>(state);
  
	loadtime(in, device.deviceStateChangeTime);
	loadtime(in, device.registerLastTime);
    
	in>>device.registerExpires;
    
	loadtime(in, device.keepaliveLastTime);
    
	in>>device.keepaliveExpires;
	in>>state;
	device.queryCatalogState=static_cast<QueryCatalogState>(state);
    in>>device.queryCatalogSN;
    in>>device.queryCatalogNumber;
    in>>device.queryCatalogSipTid;
	in>>state;
	device.subscribeCatalogState=static_cast<SubscribeCatalogState>(state);
    in>>device.subscribeCatalogSN;
    in>>device.subscribeCatalogNumber;
    in>>device.subscribeCatalogSipSid;
    in>>device.keepaliveTimerId;
	
	loadchannellist(in, device.channelList);
}


template<>
void save(const ExecutingInviteCmd& cmd, DBOutStream& out)
{
	out<<cmd.cmdSenderId;
    out<<cmd.deviceId;
    out<<static_cast<uint8_t>(cmd.cmdType);
    out<<cmd.cmdSeq;

	if(cmd.cmdType==ExecutingInviteCmd::CmdType::LiveStream)
	{
		const ExecutingInviteCmd::State::LiveStream& livestream_state = std::get<ExecutingInviteCmd::State::LiveStream>(cmd.state);
		out<<static_cast<uint8_t>(livestream_state);
		
		const ExecutingInviteCmd::Info::LiveStreamPtr& livestream_ptr = std::get<ExecutingInviteCmd::Info::LiveStreamPtr>(cmd.operationInfo);
		out<<livestream_ptr->sessionIdStart;
		out<<livestream_ptr->streamProto;
		out<<livestream_ptr->recvStreamIp;
		out<<livestream_ptr->recvStreamPort;
		out<<livestream_ptr->playIp;
		out<<livestream_ptr->playPort;
		out<<livestream_ptr->playProto;
		out<<livestream_ptr->sessionIdStop;

	}
	else if(cmd.cmdType==ExecutingInviteCmd::CmdType::RecordPlayback)
	{
		const ExecutingInviteCmd::State::RecordPlayback& record_playback_state = std::get<ExecutingInviteCmd::State::RecordPlayback>(cmd.state);
		out<<static_cast<uint8_t>(record_playback_state);
		
		const ExecutingInviteCmd::Info::RecordPlaybackPtr& record_playback_info_ptr = std::get<ExecutingInviteCmd::Info::RecordPlaybackPtr>(cmd.operationInfo);                                                                                   
        out<<record_playback_info_ptr->sessionIdStart;
        out<<record_playback_info_ptr->streamProto;
        out<<record_playback_info_ptr->recvStreamIp;
        out<<record_playback_info_ptr->recvStreamPort;
        out<<record_playback_info_ptr->playTime;
        out<<record_playback_info_ptr->endTime;
        out<<record_playback_info_ptr->playIp;
        out<<record_playback_info_ptr->playPort;
        out<<record_playback_info_ptr->playProto;
        out<<static_cast<uint8_t>(record_playback_info_ptr->playAction);
        out<<record_playback_info_ptr->scale;
        out<<record_playback_info_ptr->seekTime;
        out<<record_playback_info_ptr->sipTidPlay;
        out<<record_playback_info_ptr->sessionIdStop;
		
	}
	else if(cmd.cmdType==ExecutingInviteCmd::CmdType::RecordDownload)
	{
		const ExecutingInviteCmd::State::RecordDownload& record_download_state = std::get<ExecutingInviteCmd::State::RecordDownload>(cmd.state);
		out<<static_cast<uint8_t>(record_download_state);
		
		const ExecutingInviteCmd::Info::RecordDownloadPtr& record_download_ptr = std::get<ExecutingInviteCmd::Info::RecordDownloadPtr>(cmd.operationInfo);
		out<<record_download_ptr->sessionIdStart;
		out<<record_download_ptr->streamProto;
		out<<record_download_ptr->recvStreamIp;
		out<<record_download_ptr->recvStreamPort;
		out<<record_download_ptr->playTime;
		out<<record_download_ptr->endTime;
		out<<record_download_ptr->speed;
		out<<record_download_ptr->playIp;
		out<<record_download_ptr->playPort;
		out<<record_download_ptr->playProto;
		out<<record_download_ptr->sessionIdStop;
	}
	else if(cmd.cmdType==ExecutingInviteCmd::CmdType::VoiceBroadcast)
	{
		const ExecutingInviteCmd::State::VoiceBroadcast& voice_broadcast_state = std::get<ExecutingInviteCmd::State::VoiceBroadcast>(cmd.state);
		out<<static_cast<uint8_t>(voice_broadcast_state);
		
		const ExecutingInviteCmd::Info::VoiceBroadcastPtr& voice_broadcast_ptr = std::get<ExecutingInviteCmd::Info::VoiceBroadcastPtr>(cmd.operationInfo);
		(void)voice_broadcast_ptr;	
	}

    out<<cmd.localDeviceId;
    out<<cmd.localSipIp;
    out<<cmd.deviceSipIp;
    out<<cmd.deviceSipPort;
    out<<cmd.deviceSipPort;
    out<<cmd.sipCid;
    out<<cmd.sipDid;
    out<<cmd.sipTidBye;
    out<<cmd.sipMsgCallIdNumber;
    out<<cmd.sipMsgFromTag;
    out<<cmd.sipMsgToTag;
    out<<cmd.sipMsgCseqNumber;
}

template<>
void load(DBInStream& in, ExecutingInviteCmd& cmd)
{
	in>>cmd.cmdSenderId;
    in>>cmd.deviceId;
	uint8_t type;
	in>>type;
	cmd.cmdType=static_cast<ExecutingInviteCmd::CmdType>(type);
    in>>cmd.cmdSeq;

	if(cmd.cmdType==ExecutingInviteCmd::CmdType::LiveStream)
	{
		uint8_t state;
		in>>state;
		cmd.state=static_cast<ExecutingInviteCmd::State::LiveStream>(state);
		
		ExecutingInviteCmd::Info::LiveStreamPtr livestream_ptr=std::make_unique<ExecutingInviteCmd::Info::LiveStream>();
		in>>livestream_ptr->sessionIdStart;
		in>>livestream_ptr->streamProto;
		in>>livestream_ptr->recvStreamIp;
		in>>livestream_ptr->recvStreamPort;
		in>>livestream_ptr->playIp;
		in>>livestream_ptr->playPort;
		in>>livestream_ptr->playProto;
		in>>livestream_ptr->sessionIdStop;

		cmd.operationInfo=std::move(livestream_ptr);
	}
	else if(cmd.cmdType==ExecutingInviteCmd::CmdType::RecordPlayback)
	{
		uint8_t state;
        in>>state;
        cmd.state=static_cast<ExecutingInviteCmd::State::RecordPlayback>(state);

		ExecutingInviteCmd::Info::RecordPlaybackPtr record_playback_ptr=std::make_unique<ExecutingInviteCmd::Info::RecordPlayback>();                                                                                                  
        in>>record_playback_ptr->sessionIdStart;
        in>>record_playback_ptr->streamProto;
        in>>record_playback_ptr->recvStreamIp;
        in>>record_playback_ptr->recvStreamPort;
        in>>record_playback_ptr->playTime;
        in>>record_playback_ptr->endTime;
        in>>record_playback_ptr->playIp;
        in>>record_playback_ptr->playPort;
        in>>record_playback_ptr->playProto;
        
        uint8_t play_action;
        in>>play_action;
        record_playback_ptr->playAction=static_cast<ExecutingInviteCmd::Info::RecordPlayback::PlayAction>(play_action);

        in>>record_playback_ptr->scale;
        in>>record_playback_ptr->seekTime;
        in>>record_playback_ptr->sipTidPlay;
        in>>record_playback_ptr->sessionIdStop;

        cmd.operationInfo=std::move(record_playback_ptr);		
	}
	else if(cmd.cmdType==ExecutingInviteCmd::CmdType::RecordDownload)
	{
		uint8_t record_download_state;
		in>>record_download_state;
		cmd.state=static_cast<ExecutingInviteCmd::State::RecordDownload>(record_download_state);
		
		ExecutingInviteCmd::Info::RecordDownloadPtr record_download_ptr=std::make_unique<ExecutingInviteCmd::Info::RecordDownload>();
		in>>record_download_ptr->sessionIdStart;
		in>>record_download_ptr->streamProto;
		in>>record_download_ptr->recvStreamIp;
		in>>record_download_ptr->recvStreamPort;
		in>>record_download_ptr->playTime;
		in>>record_download_ptr->endTime;
		in>>record_download_ptr->speed;
		in>>record_download_ptr->playIp;
		in>>record_download_ptr->playPort;
		in>>record_download_ptr->playProto;
		in>>record_download_ptr->sessionIdStop;

		cmd.operationInfo=std::move(record_download_ptr);
	}
	else if(cmd.cmdType==ExecutingInviteCmd::CmdType::VoiceBroadcast)
	{
		uint8_t voice_broadcast_state;
		in>>voice_broadcast_state;
		cmd.state=static_cast<ExecutingInviteCmd::State::VoiceBroadcast>(voice_broadcast_state);
		
		ExecutingInviteCmd::Info::VoiceBroadcastPtr voice_broadcast_ptr=std::make_unique<ExecutingInviteCmd::Info::VoiceBroadcast>();	
		cmd.operationInfo=std::move(voice_broadcast_ptr);
	}

    in>>cmd.localDeviceId;
    in>>cmd.localSipIp;
    in>>cmd.deviceSipIp;
    in>>cmd.deviceSipPort;
    in>>cmd.deviceSipPort;
    in>>cmd.sipCid;
    in>>cmd.sipDid;
    in>>cmd.sipTidBye;
    in>>cmd.sipMsgCallIdNumber;
    in>>cmd.sipMsgFromTag;
    in>>cmd.sipMsgToTag;
    in>>cmd.sipMsgCseqNumber;
} 

} // namespace GBGateway

