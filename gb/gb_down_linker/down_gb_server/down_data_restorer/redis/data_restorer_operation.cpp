#include"data_restorer_operation.h"
#include"redis_client_util.h"
#include<cstring>
#include<sstream>

namespace GBGateway {


Device* CopyDevice(Device* d)
{
	if(d==NULL)
	{
		return NULL;
	}

	Device* device=new Device();
	if(device==NULL)
		return device;

	device->deviceId               = d->deviceId;
    device->deviceSipIp            = d->deviceSipIp;
    device->deviceSipPort          = d->deviceSipPort;
    device->deviceState            = d->deviceState;
    if (d->deviceStateChangeTime != NULL)
    {   
        device->deviceStateChangeTime = d->deviceStateChangeTime->Clone();
    }  
	else
	{
		device->deviceStateChangeTime = NULL; 
	}
    if (d->registerLastTime != NULL)
    {   
        device->registerLastTime = d->registerLastTime->Clone();
    }   
	else
	{
		device->registerLastTime = NULL;
	}
    device->registerExpires        = d->registerExpires;
    if (d->keepaliveLastTime != NULL)
    {   
        device->keepaliveLastTime = d->keepaliveLastTime->Clone();
    }   
	else
		device->keepaliveLastTime = NULL;

    device->keepaliveExpires       = d->keepaliveExpires;
    device->queryCatalogState      = d->queryCatalogState;
    device->queryCatalogSN         = d->queryCatalogSN;
    device->queryCatalogNumber     = d->queryCatalogNumber;
    device->queryCatalogSipTid     = d->queryCatalogSipTid;
    device->subscribeCatalogState  = d->subscribeCatalogState;
    device->subscribeCatalogSN     = d->subscribeCatalogSN;
	device->subscribeCatalogNumber = d->subscribeCatalogNumber;
    device->subscribeCatalogSipSid = d->subscribeCatalogSipSid;

    if (d->channelList != NULL)
    {
        device->channelList = d->channelList->Clone();
    }
	else
		device->channelList = NULL;

    device->keepaliveTimerId = d->keepaliveTimerId;

	device->deviceSipIp="";
	device->deviceSipPort="";
	device->deviceState = DeviceState::Registered;
	device->deviceStateChangeTime=NULL;
	device->registerLastTime=NULL;
	device->registerExpires = 0;
	device->keepaliveLastTime=NULL;
	device->keepaliveExpires = 0;
	device->queryCatalogState = QueryCatalogState::None;
	device->queryCatalogSN = 0;
	device->queryCatalogNumber = 0;
	device->queryCatalogSipTid = 0;
	device->subscribeCatalogState = SubscribeCatalogState::None;
	device->subscribeCatalogSN = 0;
	device->subscribeCatalogNumber = 0;
	device->subscribeCatalogSipSid = 0;
	device->keepaliveTimerId = 0;
	device->channelList=NULL;

	return device;
}

Channel* CopyChannel(Channel* c)
{
	Channel* channel=new Channel();
	if (channel != NULL)
    {   
        channel->deviceId            = c->deviceId;
        channel->channelDeviceId     = c->channelDeviceId;
        channel->channelName         = c->channelName;
        channel->channelManufacturer = c->channelManufacturer;
        channel->channelModel        = c->channelModel;
        channel->channelOwner        = c->channelOwner;
        channel->channelCivilCode    = c->channelCivilCode;
        channel->channelAddress      = c->channelAddress;
        channel->channelParental     = c->channelParental;
        channel->channelParentId     = c->channelParentId;
        channel->channelSafetyWay    = c->channelSafetyWay;
        channel->channelRegisterWay  = c->channelRegisterWay;
        channel->channelSecrecy      = c->channelSecrecy;

        channel->channelStatus       = c->channelStatus;
    }   

    return channel;
}

ExecutingInviteCmd* CopyExecutingInviteCmd(ExecutingInviteCmd* invite)
{
	ExecutingInviteCmd* cmd=new ExecutingInviteCmd();
	if(cmd==NULL)
		return cmd;
	
	cmd->cmdSenderId=invite->cmdSenderId;
	cmd->deviceId=invite->deviceId;
	cmd->cmdType=invite->cmdType;
	cmd->cmdSeq=invite->cmdSeq;
	cmd->state=invite->state;

	if(invite->cmdType==ExecutingInviteCmd::CmdType::LiveStream)
	{
		const ExecutingInviteCmd::Info::LiveStreamPtr& invite_livestream_ptr=std::get<ExecutingInviteCmd::Info::LiveStreamPtr>(invite->operationInfo);

		ExecutingInviteCmd::Info::LiveStreamPtr livestream_ptr=std::make_unique<ExecutingInviteCmd::Info::LiveStream>();
		if(livestream_ptr!=nullptr  &&  invite_livestream_ptr!=nullptr)
		{
			livestream_ptr->sessionIdStart=invite_livestream_ptr->sessionIdStart;
			livestream_ptr->streamProto=invite_livestream_ptr->streamProto;
			livestream_ptr->recvStreamIp=invite_livestream_ptr->recvStreamIp;
			livestream_ptr->recvStreamPort=invite_livestream_ptr->recvStreamPort;
			livestream_ptr->playIp=invite_livestream_ptr->playIp;
			livestream_ptr->playPort=invite_livestream_ptr->playPort;
			livestream_ptr->playProto=invite_livestream_ptr->playProto;
			livestream_ptr->sessionIdStop=invite_livestream_ptr->sessionIdStop;
		}

		cmd->operationInfo=std::move(livestream_ptr);
	}  
	else if (invite->cmdType == ExecutingInviteCmd::CmdType::RecordPlayback)
	{
		const ExecutingInviteCmd::Info::RecordPlaybackPtr& invite_record_playback_ptr = std::get<ExecutingInviteCmd::Info::RecordPlaybackPtr>(invite->operationInfo);

		ExecutingInviteCmd::Info::RecordPlaybackPtr record_playback_ptr = std::make_unique<ExecutingInviteCmd::Info::RecordPlayback>();

		if(invite_record_playback_ptr!=nullptr  &&  record_playback_ptr!=nullptr)
		{
			record_playback_ptr->sessionIdStart = invite_record_playback_ptr->sessionIdStart;
			record_playback_ptr->streamProto = invite_record_playback_ptr->streamProto;
			record_playback_ptr->recvStreamIp = invite_record_playback_ptr->recvStreamIp;
			record_playback_ptr->recvStreamPort = invite_record_playback_ptr->recvStreamPort;
			record_playback_ptr->playTime = invite_record_playback_ptr->playTime;
			record_playback_ptr->endTime = invite_record_playback_ptr->endTime;
			record_playback_ptr->playIp = invite_record_playback_ptr->playIp;
			record_playback_ptr->playPort = invite_record_playback_ptr->playPort;
			record_playback_ptr->playProto  = invite_record_playback_ptr->playProto;

			record_playback_ptr->playAction = invite_record_playback_ptr->playAction;

			record_playback_ptr->scale = invite_record_playback_ptr->scale;
			record_playback_ptr->seekTime = invite_record_playback_ptr->seekTime;
			record_playback_ptr->sipTidPlay = invite_record_playback_ptr->sipTidPlay;
			record_playback_ptr->sessionIdStop = invite_record_playback_ptr->sessionIdStop;
		}

		cmd->operationInfo = std::move(record_playback_ptr);
	}
	else if (invite->cmdType == ExecutingInviteCmd::CmdType::RecordDownload)
	{
		const ExecutingInviteCmd::Info::RecordDownloadPtr& invite_record_download_ptr = std::get<ExecutingInviteCmd::Info::RecordDownloadPtr>(invite->operationInfo);

		ExecutingInviteCmd::Info::RecordDownloadPtr record_download_ptr = std::make_unique<ExecutingInviteCmd::Info::RecordDownload>();

		if(invite_record_download_ptr!=nullptr  &&  record_download_ptr!=nullptr)
		{
			record_download_ptr->sessionIdStart = invite_record_download_ptr->sessionIdStart;
			record_download_ptr->streamProto = invite_record_download_ptr->streamProto;
			record_download_ptr->recvStreamIp = invite_record_download_ptr->recvStreamIp;
			record_download_ptr->recvStreamPort = invite_record_download_ptr->recvStreamPort;
			record_download_ptr->playTime = invite_record_download_ptr->playTime;
			record_download_ptr->endTime = invite_record_download_ptr->endTime;
			record_download_ptr->speed = invite_record_download_ptr->speed;
			record_download_ptr->playIp = invite_record_download_ptr->playIp;
			record_download_ptr->playPort = invite_record_download_ptr->playPort;
			record_download_ptr->playProto =invite_record_download_ptr->playProto;
			record_download_ptr->sessionIdStop = invite_record_download_ptr->sessionIdStop;
		}

		cmd->operationInfo = std::move(record_download_ptr);
	}
	else if (invite->cmdType == ExecutingInviteCmd::CmdType::VoiceBroadcast)
	{
		ExecutingInviteCmd::Info::VoiceBroadcastPtr voice_broadcast_ptr = std::make_unique<ExecutingInviteCmd::Info::VoiceBroadcast>();

		cmd->operationInfo = std::move(voice_broadcast_ptr);
	}

	cmd->localDeviceId=invite->localDeviceId;
	cmd->localSipIp=invite->localSipIp;
	cmd->deviceSipIp=invite->deviceSipIp;
	cmd->deviceSipPort=invite->deviceSipPort;
	cmd->sipCid=invite->sipCid;
	cmd->sipDid=invite->sipDid;
	cmd->sipTidBye=invite->sipTidBye;
	cmd->sipMsgCallIdNumber=invite->sipMsgCallIdNumber;
	cmd->sipMsgFromTag=invite->sipMsgFromTag;
	cmd->sipMsgToTag=invite->sipMsgToTag;
	cmd->sipMsgCseqNumber=invite->sipMsgCseqNumber;
	
	return cmd;
}

bool PushEntity(DataRestorerOperation& operation, Device* device)
{
	if (device == NULL)
		return false;
	Device* copy = CopyDevice(device);
	if(copy==NULL)
		return false;

	operation.entities.push_back(copy);
	return true;
}

bool PushEntity(DataRestorerOperation& operation, Channel* channel)
{
	if (channel == NULL)
		return false;
	Channel* copy = CopyChannel(channel);
	if(copy==NULL)
		return false;
	operation.entities.push_back(copy);
	return true;
}

bool PushEntity(DataRestorerOperation& operation, ExecutingInviteCmd* executing_invite_cmd)
{
	if (executing_invite_cmd == NULL)
		return false;
	ExecutingInviteCmd* copy = CopyExecutingInviteCmd(executing_invite_cmd);
	if(copy==NULL)
		return false;
	operation.entities.push_back(copy);
	return true;
}

} // namespace GBGateway
