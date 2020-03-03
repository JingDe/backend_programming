#include"data_restorer_operation.h"

namespace GBGateway {
	
Device* DeviceCopy(Device* d)
{
	Device* device=(Device*)malloc(sizeof(Device));
	
	device->deviceId               = d->deviceId;
    device->deviceSipIp            = d->deviceSipIp;
    device->deviceSipPort          = d->deviceSipPort;
    device->deviceState            = d->deviceState;
    if (d->deviceStateChangeTime != nullptr)
    {   
        device->deviceStateChangeTime = d->deviceStateChangeTime->Clone();
    }   
    if (d->registerLastTime != nullptr)
    {   
        device->registerLastTime = d->registerLastTime->Clone();
    }   
    device->registerExpires        = d->registerExpires;
    if (d->keepaliveLastTime != nullptr)
    {   
        device->keepaliveLastTime = d->keepaliveLastTime->Clone();
    }   
    device->keepaliveExpires       = d->keepaliveExpires;
    device->queryCatalogState      = d->queryCatalogState;
    device->queryCatalogSN         = d->queryCatalogSN;
    device->queryCatalogNumber     = d->queryCatalogNumber;
    device->queryCatalogSipTid     = d->queryCatalogSipTid;
    device->subscribeCatalogState  = d->subscribeCatalogState;
    device->subscribeCatalogSN     = d->subscribeCatalogSN;
	device->subscribeCatalogNumber = d->subscribeCatalogNumber;
    device->subscribeCatalogSipSid = d->subscribeCatalogSipSid;

    if (d->channelList != nullptr)
    {
        device->channelList = d->channelList->Clone();
    }

    device->keepaliveTimerId = d->keepaliveTimerId;
	
	return device;
}

Channel* ChannelCopy(Channel* c)
{
	Channel* channel=(Channel*)malloc(sizeof(Channel));
	if (channel != nullptr)
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

ExecutingInviteCmd* ExecutingInviteCmdCopy(ExecutingInviteCmd* eic)
{
	ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)malloc(sizeof(ExecutingInviteCmd));
	if(cmd==nullptr)
		return cmd;
	
	cmd->cmdSenderId=eic->cmdSenderId;
	cmd->deviceId=eic->deviceId;
	cmd->cmdType=eic->cmdType;
	cmd->cmdSeq=eic->cmdSeq;
	
	// TODO
	
	return cmd;
}


} // namespace GBGateway
