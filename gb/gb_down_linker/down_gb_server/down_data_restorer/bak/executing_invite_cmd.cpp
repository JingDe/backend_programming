#include"executing_invite_cmd.h"

namespace GBGateway {

std::unique_ptr<ExecutingInviteCmd> ExecutingInviteCmd::Clone()
{
	std::unique_ptr<ExecutingInviteCmd> cmd=std::make_unique<ExecutingInviteCmd>();
	if(cmd==nullptr)
		return cmd;

	cmd->cmdSenderId=cmdSenderId;
	cmd->deviceId=deviceId;
	cmd->cmdType=cmdType;
	cmd->cmdSeq=cmdSeq;


	cmd->state=state;

	if(cmdType==ExecutingInviteCmd::CmdType::LiveStream)
	{
		const ExecutingInviteCmd::Info::LiveStreamPtr& operation_info  = std::get<ExecutingInviteCmd::Info::LiveStreamPtr>(operationInfo);

		ExecutingInviteCmd::Info::LiveStreamPtr info = std::make_unique<ExecutingInviteCmd::Info::LiveStream>();

		info->sessionIdStart	= operation_info->sessionIdStart;
		info->streamProto       = operation_info->streamProto;
		info->recvStreamIp      = operation_info->recvStreamIp;
		info->recvStreamPort    = operation_info->recvStreamPort;
		info->playIp			= operation_info->playIp;
		info->playPort			= operation_info->playPort;
		info->playProto			= operation_info->playProto;
		info->sessionIdStop		= operation_info->sessionIdStop;

		cmd->operationInfo = std::move(info);            
	}
	else if(cmdType==ExecutingInviteCmd::CmdType::RecordPlayback)
	{
		const ExecutingInviteCmd::Info::RecordPlaybackPtr& operation_info  = std::get<ExecutingInviteCmd::Info::RecordPlaybackPtr>(operationInfo);

		ExecutingInviteCmd::Info::RecordPlaybackPtr info = std::make_unique<ExecutingInviteCmd::Info::RecordPlayback>();
		info->sessionIdStart     = operation_info->sessionIdStart;
		info->streamProto        = operation_info->streamProto;
		info->recvStreamIp       = operation_info->recvStreamIp;
		info->recvStreamPort     = operation_info->recvStreamPort;
		info->playTime           = operation_info->playTime;
		info->endTime            = operation_info->endTime;
		info->playIp     		 = operation_info->playIp;
		info->playPort           = operation_info->playPort;
		info->playProto          = operation_info->playProto;
		info->playAction         = operation_info->playAction;
		info->scale              = operation_info->scale;
		info->seekTime           = operation_info->seekTime;
		info->sipTidPlay         = operation_info->sipTidPlay;
		info->sessionIdStop      = operation_info->sessionIdStop;

		cmd->operationInfo = std::move(info);
	}
	else if(cmdType==ExecutingInviteCmd::CmdType::RecordDownload)
	{
		const ExecutingInviteCmd::Info::RecordDownloadPtr& operation_info  = std::get<ExecutingInviteCmd::Info::RecordDownloadPtr>(operationInfo);

		ExecutingInviteCmd::Info::RecordDownloadPtr info = std::make_unique<ExecutingInviteCmd::Info::RecordDownload>();
	
		info->sessionIdStart	= operation_info->sessionIdStart;
		info->streamProto		= operation_info->streamProto;
		info->recvStreamIp		= operation_info->recvStreamIp;
		info->recvStreamPort	= operation_info->recvStreamPort;
		info->playTime			= operation_info->playTime;
		info->endTime			= operation_info->endTime;
		info->speed				= operation_info->speed;
		info->playIp			= operation_info->playIp;
		info->playPort			= operation_info->playPort;
		info->playProto			= operation_info->playProto;
		info->sessionIdStop		= operation_info->sessionIdStop;

		cmd->operationInfo = std::move(info);
	}
	else if(cmdType==ExecutingInviteCmd::CmdType::VoiceBroadcast)
	{
		//const ExecutingInviteCmd::Info::VoiceBroadcastPtr& operation_info  = std::get<ExecutingInviteCmd::Info::VoiceBroadcastPtr>(operationInfo);

		ExecutingInviteCmd::Info::VoiceBroadcastPtr info = std::make_unique<ExecutingInviteCmd::Info::VoiceBroadcast>();

		//void(operation_info);

		cmd->operationInfo = std::move(info);
	}


	cmd->localDeviceId=localSipIp;
	cmd->deviceSipIp=deviceSipIp;
	cmd->deviceSipPort=deviceSipPort;
	cmd->sipCid=sipCid;
	cmd->sipDid=sipDid;
	cmd->sipTidBye=sipTidBye;
	cmd->sipMsgCallIdNumber=sipMsgCallIdNumber;
	cmd->sipMsgFromTag=sipMsgFromTag;
	cmd->sipMsgToTag=sipMsgToTag;
	cmd->sipMsgCseqNumber=sipMsgCseqNumber;

	return cmd;
}

ExecutingInviteCmd& ExecutingInviteCmd::operator=(const ExecutingInviteCmd& eic)
{
	cmdSenderId=eic.cmdSenderId;
	deviceId=eic.deviceId;
	cmdType=eic.cmdType;
	cmdSeq=eic.cmdSeq;
	state=eic.state;

	if(cmdType==ExecutingInviteCmd::CmdType::LiveStream)
	{
		const ExecutingInviteCmd::Info::LiveStreamPtr& operation_info  = std::get<ExecutingInviteCmd::Info::LiveStreamPtr>(eic.operationInfo);

		ExecutingInviteCmd::Info::LiveStreamPtr info = std::make_unique<ExecutingInviteCmd::Info::LiveStream>();

		info->sessionIdStart     = operation_info->sessionIdStart;
		info->streamProto        = operation_info->streamProto;
		info->recvStreamIp       = operation_info->recvStreamIp;
		info->recvStreamPort     = operation_info->recvStreamPort;
		info->playIp			= operation_info->playIp;
		info->playPort			= operation_info->playPort;
		info->playProto			= operation_info->playProto;
		info->sessionIdStop		= operation_info->sessionIdStop;

		operationInfo = std::move(info);            
	}
	else if(cmdType==ExecutingInviteCmd::CmdType::RecordPlayback)
	{
		const ExecutingInviteCmd::Info::RecordPlaybackPtr& operation_info  = std::get<ExecutingInviteCmd::Info::RecordPlaybackPtr>(eic.operationInfo);

		ExecutingInviteCmd::Info::RecordPlaybackPtr info = std::make_unique<ExecutingInviteCmd::Info::RecordPlayback>();

		info->sessionIdStart     = operation_info->sessionIdStart;
		info->streamProto        = operation_info->streamProto;
		info->recvStreamIp       = operation_info->recvStreamIp;
		info->recvStreamPort     = operation_info->recvStreamPort;
		info->playTime           = operation_info->playTime;
		info->endTime            = operation_info->endTime;
		info->playIp     		 = operation_info->playIp;
		info->playPort           = operation_info->playPort;
		info->playProto          = operation_info->playProto;
		info->playAction         = operation_info->playAction;
		info->scale              = operation_info->scale;
		info->seekTime           = operation_info->seekTime;
		info->sipTidPlay         = operation_info->sipTidPlay;
		info->sessionIdStop      = operation_info->sessionIdStop;

		operationInfo = std::move(info);
	}
	else if(cmdType==ExecutingInviteCmd::CmdType::RecordDownload)
	{
		const ExecutingInviteCmd::Info::RecordDownloadPtr& operation_info  = std::get<ExecutingInviteCmd::Info::RecordDownloadPtr>(eic.operationInfo);

		ExecutingInviteCmd::Info::RecordDownloadPtr info = std::make_unique<ExecutingInviteCmd::Info::RecordDownload>();

		info->sessionIdStart	= operation_info->sessionIdStart;
		info->streamProto		= operation_info->streamProto;
		info->recvStreamIp		= operation_info->recvStreamIp;
		info->recvStreamPort	= operation_info->recvStreamPort;
		info->playTime			= operation_info->playTime;
		info->endTime			= operation_info->endTime;
		info->speed				= operation_info->speed;
		info->playIp			= operation_info->playIp;
		info->playPort			= operation_info->playPort;
		info->playProto			= operation_info->playProto;
		info->sessionIdStop		= operation_info->sessionIdStop;

		operationInfo = std::move(info);
	}
	else if(cmdType==ExecutingInviteCmd::CmdType::VoiceBroadcast)
	{
		//const ExecutingInviteCmd::Info::VoiceBroadcastPtr& operation_info  = std::get<ExecutingInviteCmd::Info::VoiceBroadcastPtr>(eic.operationInfo);

		ExecutingInviteCmd::Info::VoiceBroadcastPtr info = std::make_unique<ExecutingInviteCmd::Info::VoiceBroadcast>();

		//void(operation_info);

		operationInfo = std::move(info);
	}	

	localDeviceId=eic.localSipIp;
	deviceSipIp=eic.deviceSipIp;
	deviceSipPort=eic.deviceSipPort;
	sipCid=eic.sipCid;
	sipDid=eic.sipDid;
	sipTidBye=eic.sipTidBye;
	sipMsgCallIdNumber=eic.sipMsgCallIdNumber;
	sipMsgFromTag=eic.sipMsgFromTag;
	sipMsgToTag=eic.sipMsgToTag;
	sipMsgCseqNumber=eic.sipMsgCseqNumber;
	return *this;	
}

} // namespace GBGateway
