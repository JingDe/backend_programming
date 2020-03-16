#ifndef DOWN_DATA_RESTORER_DBSERIALIZE_H
#define DOWN_DATA_RESTORER_DBSERIALIZE_H

#include"dbstream.h"
#include"channel.h"
#include"device.h"
#include"executing_invite_cmd.h"

namespace GBGateway {

template<typename T> 
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e) 
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

class DBSerialize{
public:

	template<typename T>
	void save(const T& entity, DBOutputStream& out);

	template<typename T>
	void load(T& entity, DBInputStream& in);
	
private:
	void savetime(const TimePtr& timeptr, DBOutputStream& out);
	void loadtime(DBInputStream& in, TimePtr& timeptr);
	void savechannellist(const ChannelListPtr& channellist, DBOutputStream& out);
	void loadchannellist(DBInputStream& in, ChannelListPtr& channellist);
};

template<>
void DBSerialize::save(const Channel& channel, DBOutputStream& out)
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
void DBSerialize::load(DBInputStream& in, Channel& channel)
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
	in>>static_cast<uint8_t>(channel.channelStatus);
}

void DBSerialize::savetime(const TimePtr& timeptr, DBOutputStream& out)
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
		out<<tmptr->tm_iddst;
	}
}

void DBSerialize::loadtime(DBInputStream& in, TimePtr& timeptr)
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
		struct tm* time=(struct tm*)malloc(sizeof(struct tm))
		in>>time->tm_sec;
		in>>time->tm_min;
		in>>time->tm_hour;
		in>>time->tm_mday;
		in>>time->tm_mon;
		in>>time->tm_year;
		in>>time->tm_wday;
		in>>time->tm_yday;
		in>>time->tm_iddst;
		
		timeptr=std::make_unique<CTime>(time);
	}
}

void DBSerialize::savechannellist(const ChannelListPtr& channellist, DBOutputStream& out)
{
	ChannelListPtr channel_list=channellist->Clone();
	std::list<ChannelPtr>& channelptr_list=channel_list->GetChannelList();
	
	out<<static_cast<uint32_t>(channel_list.size());
	
	for(ChannelPtr& channel_ptr : channelptr_list)
	{
		out<<channel_ptr->deviceId;
		out<<channel_ptr->channelDeviceId;
	}
}

void DBSerialize::loadchannellist(DBInputStream& in, ChannelListPtr& channellist)
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
		channel->deviceId=deviceId;
		channel->channelDeviceId=channelDeviceId;
		channellist->GetChannelList().push_back(channel_ptr);
	}
}

template<>
void DBSerialize::save(const Device& device, DBOutputStream& out)
{
	out<<device.deviceId;
    out<<device.deviceSipIp;
    out<<device.deviceSipPort;
    out<<static_cast<uint8_t>(device.deviceState);
	
	savetime(device.deviceStateChangeTime, out);
	savetime(device.registerLastTime, out);
    
	out<<device.registerExpires;
    
	savetime(device.keepaliveLastTime);
    
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
void DBSerialize::load(DBInputStream& in, Device& device)
{
	in>>device.deviceId;
    in>>device.deviceSipIp;
    in>>device.deviceSipPort;
    in>>static_cast<uint8_t>(device.deviceState);
	
	loadtime(device.deviceStateChangeTime, out);
	loadtime(device.registerLastTime, out);
    
	in>>device.registerExpires;
    
	loadtime(device.keepaliveLastTime);
    
	in>>device.keepaliveExpires;
	in>>static_cast<uint8_t>(device.queryCatalogState);
    in>>device.queryCatalogSN;
    in>>device.queryCatalogNumber;
    in>>device.queryCatalogSipTid;
	in>>static_cast<uint8_t>(device.subscribeCatalogState);
    in>>device.subscribeCatalogSN;
    in>>device.subscribeCatalogNumber;
    in>>device.subscribeCatalogSipSid;
    in>>device.keepaliveTimerId;
	
	loadchannellist(in, device.channelList);
}


template<>
void DBSerialize::save(const ExecutingInviteCmd& cmd, DBOutputStream& out)
{
	out<<cmd.cmdSenderId;
    out<<cmd.deviceId;
    out<<static_cast<uint8_t>(cmd.cmdType);
    out<<cmd.cmdSeq;

	if(cmd.Type==ExecutingInviteCmd::CmdType::LiveStream)
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
	else if(cmd.Type==ExecutingInviteCmd::CmdType::RecordPlayback)
	{
		const ExecutingInviteCmd::State::RecordPlayback& record_playback_state = std::get<ExecutingInviteCmd::State::RecordPlayback>(cmd.state);
		out<<static_cast<uint8_t>(record_playback_state);
		
		const ExecutingInviteCmd::Info::RecordPlaybackPtr& livestream_ptr = std::get<ExecutingInviteCmd::Info::RecordPlaybackPtr>(cmd.operationInfo);		
		out<<livestream_ptr->sessionIdStart;
        out<<livestream_ptr->streamProto;
        out<<livestream_ptr->recvStreamIp;
        out<<livestream_ptr->recvStreamPort;
        out<<livestream_ptr->playTime;
        out<<livestream_ptr->endTime;
		out<<livestream_ptr->playIp;
		out<<livestream_ptr->playPort;
		out<<livestream_ptr->playProto;
		out<<static_cast<uint8_t>(livestream_ptr->playAction);
		out<<livestream_ptr->scale;
		out<<livestream_ptr->seekTime;
		out<<livestream_ptr->sipTidPlay;
		out<<livestream_ptr->sessionIdStop;
	}
	else if(cmd.Type==ExecutingInviteCmd::CmdType::RecordDownload)
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
	else if(cmd.Type==ExecutingInviteCmd::CmdType::VoiceBroadcast)
	{
		const ExecutingInviteCmd::State::VoiceBroadcast& voice_broadcast_state = std::get<ExecutingInviteCmd::State::VoiceBroadcast>(cmd.state);
		out<<static_cast<uint8_t>(voice_broadcast_state);
		
		const ExecutingInviteCmd::Info::VoiceBroadcastPtr& voice_broadcast_ptr = std::get<ExecutingInviteCmd::Info::VoiceBroadcastPtr>(cmd.operationInfo);
		
	}

    out<<cmd.localDeviceId;
    out<<cmd.localSipIp;
    out<<cmd.deviceSipIp;
    out<<cmd.deviceSipPort;
    out<<cmd.deviceSipPort;
    out<<sipCid;
    out<<sipDid;
    out<<sipTidBye;
    out<<sipMsgCallIdNumber;
    out<<sipMsgFromTag;
    out<<sipMsgToTag;
    out<<sipMsgCseqNumber;
}

template<>
void DBSerialize::load(DBInputStream& in, ExecutingInviteCmd& cmd)
{
	in>>cmd.cmdSenderId;
    in>>cmd.deviceId;
    in>>static_cast<uint8_t>(cmd.cmdType);
    in>>cmd.cmdSeq;

	if(cmd.Type==ExecutingInviteCmd::CmdType::LiveStream)
	{
		ExecutingInviteCmd::State::LiveStream& livestream_state = std::get<ExecutingInviteCmd::State::LiveStream>(cmd.state);
		in>>static_cast<uint8_t>(livestream_state);
		
		ExecutingInviteCmd::Info::LiveStreamPtr& livestream_ptr = std::get<ExecutingInviteCmd::Info::LiveStreamPtr>(cmd.operationInfo);
		in>>livestream_ptr->sessionIdStart;
		in>>livestream_ptr->streamProto;
		in>>livestream_ptr->recvStreamIp;
		in>>livestream_ptr->recvStreamPort;
		in>>livestream_ptr->playIp;
		in>>livestream_ptr->playPort;
		in>>livestream_ptr->playProto;
		in>>livestream_ptr->sessionIdStop;

	}
	else if(cmd.Type==ExecutingInviteCmd::CmdType::RecordPlayback)
	{
		ExecutingInviteCmd::State::RecordPlayback& record_playback_state = std::get<ExecutingInviteCmd::State::RecordPlayback>(cmd.state);
		in>>static_cast<uint8_t>(record_playback_state);
		
		ExecutingInviteCmd::Info::RecordPlaybackPtr& livestream_ptr = std::get<ExecutingInviteCmd::Info::RecordPlaybackPtr>(cmd.operationInfo);		
		in>>livestream_ptr->sessionIdStart;
        in>>livestream_ptr->streamProto;
        in>>livestream_ptr->recvStreamIp;
        in>>livestream_ptr->recvStreamPort;
        in>>livestream_ptr->playTime;
        in>>livestream_ptr->endTime;
		in>>livestream_ptr->playIp;
		in>>livestream_ptr->playPort;
		in>>livestream_ptr->playProto;
		in>>static_cast<uint8_t>(livestream_ptr->playAction);
		in>>livestream_ptr->scale;
		in>>livestream_ptr->seekTime;
		in>>livestream_ptr->sipTidPlay;
		in>>livestream_ptr->sessionIdStop;
	}
	else if(cmd.Type==ExecutingInviteCmd::CmdType::RecordDownload)
	{
		ExecutingInviteCmd::State::RecordDownload& record_download_state = std::get<ExecutingInviteCmd::State::RecordDownload>(cmd.state);
		in>>static_cast<uint8_t>(record_download_state);
		
		ExecutingInviteCmd::Info::RecordDownloadPtr& record_download_ptr = std::get<ExecutingInviteCmd::Info::RecordDownloadPtr>(cmd.operationInfo);
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
	}
	else if(cmd.Type==ExecutingInviteCmd::CmdType::VoiceBroadcast)
	{
		ExecutingInviteCmd::State::VoiceBroadcast& voice_broadcast_state = std::get<ExecutingInviteCmd::State::VoiceBroadcast>(cmd.state);
		in>>static_cast<uint8_t>(voice_broadcast_state);
		
		ExecutingInviteCmd::Info::VoiceBroadcastPtr& voice_broadcast_ptr = std::get<ExecutingInviteCmd::Info::VoiceBroadcastPtr>(cmd.operationInfo);
		
	}

    in>>cmd.localDeviceId;
    in>>cmd.localSipIp;
    in>>cmd.deviceSipIp;
    in>>cmd.deviceSipPort;
    in>>cmd.deviceSipPort;
    in>>sipCid;
    in>>sipDid;
    in>>sipTidBye;
    in>>sipMsgCallIdNumber;
    in>>sipMsgFromTag;
    in>>sipMsgToTag;
    in>>sipMsgCseqNumber;
}

} // namespace GBGateway

#endif
