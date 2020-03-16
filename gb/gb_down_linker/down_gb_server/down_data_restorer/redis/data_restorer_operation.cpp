#include"data_restorer_operation.h"
#include"redis_client_util.h"
#include<cstring>
#include<sstream>

namespace GBGateway {


Device* DeviceCopy(Device* d)
{
	if(d==nullptr)
	{
		return nullptr;
	}

	Device* device=new Device();
	if(device==nullptr)
		return device;

	device->deviceId               = d->deviceId;

    device->deviceSipIp            = d->deviceSipIp;
    device->deviceSipPort          = d->deviceSipPort;
    device->deviceState            = d->deviceState;
    if (d->deviceStateChangeTime != nullptr)
    {   
        device->deviceStateChangeTime = d->deviceStateChangeTime->Clone();
    }  
	else
	{
		device->deviceStateChangeTime = nullptr; 
	}
    if (d->registerLastTime != nullptr)
    {   
        device->registerLastTime = d->registerLastTime->Clone();
    }   
	else
	{
		device->registerLastTime = nullptr;
	}
    device->registerExpires        = d->registerExpires;
    if (d->keepaliveLastTime != nullptr)
    {   
        device->keepaliveLastTime = d->keepaliveLastTime->Clone();
    }   
	else
		device->keepaliveLastTime = nullptr;

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
	else
		device->channelList = nullptr;

    device->keepaliveTimerId = d->keepaliveTimerId;

	device->deviceSipIp="";
	device->deviceSipPort="";
	device->deviceState = DeviceState::Registered;
	device->deviceStateChangeTime=nullptr;
	device->registerLastTime=nullptr;
	device->registerExpires = 0;
	device->keepaliveLastTime=nullptr;
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
	device->channelList=nullptr;

	return device;
}

Channel* ChannelCopy(Channel* c)
{
	Channel* channel=new Channel();
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
	ExecutingInviteCmd* cmd=new ExecutingInviteCmd();
	if(cmd==nullptr)
		return cmd;
	
	cmd->cmdSenderId=eic->cmdSenderId;
	cmd->deviceId=eic->deviceId;
	cmd->cmdType=eic->cmdType;
	cmd->cmdSeq=eic->cmdSeq;
	
	// TODO
	
	return cmd;
}

void PushEntity(DataRestorerOperation operation, Device* device)
{
	if (device == NULL)
		return;
	Device* copy = DeviceCopy(device);
	operation.entities.push_back(copy);
}

void PushEntity(DataRestorerOperation operation, Channel* channel)
{
	if (channel == NULL)
		return;
	Channel* copy = ChannelCopy(channel);
	operation.entities.push_back(copy);
}

void PushEntity(DataRestorerOperation operation, ExecutingInviteCmd* executing_invite_cmd)
{
	if (executing_invite_cmd == NULL)
		return;
	ExecutingInviteCmd* copy = ExecutingInviteCmdCopy(executing_invite_cmd);
	operation.entities.push_back(copy);
}

} // namespace GBGateway
