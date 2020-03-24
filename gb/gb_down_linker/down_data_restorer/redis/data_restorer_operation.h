#ifndef DOWN_DATA_RESTORER_OPERATION_H
#define DOWN_DATA_RESTORER_OPERATION_H

#include "channel.h"
#include "device.h"
#include "executing_invite_cmd.h"
#include "base_library/log.h"

#include <cassert>
#include <list>
#include <string>


namespace GBDownLinker {

struct DataRestorerOperation;

bool PushEntity(DataRestorerOperation& operation, Device* device);
bool PushEntity(DataRestorerOperation& operation, Channel* channel);
bool PushEntity(DataRestorerOperation& operation, ExecutingInviteCmd* invite);

Device* CopyDevice(Device* d);
Channel* CopyChannel(Channel* c);
ExecutingInviteCmd* CopyExecutingInviteCmd(ExecutingInviteCmd* eic);


struct DataRestorerOperation {
	enum OperationType {
		UNKNOWN_OPERATION = 0,
		INSERT,
		UPDATE,
		DELETE,
		DELETE_CHANNEL_BY_ID,
		DELETE_CHANNEL_BY_DEVICE_ID,
		CLEAR,
	} operationType;

	enum EntityType {
		UNKNOWN_ENTITY = 0,
		DEVICE,
		CHANNEL,
		EXECUTING_INVITE_CMD,
	} entityType;

	std::string gbDownlinkerDeviceId;
	int workerThreadIdx;

	// Device
	std::string deviceId;

	// Channel
	std::string channelDeviceId;
	std::string channelChannelDeviceId;

	// ExecutingInviteCmd
	std::string inviteCmdSenderId;
	std::string inviteDeviceId;
	int64_t inviteCmdSeq;


	std::list<void*> entities; // pointer released in DownDataRestorer, after command processed

	// list<shared_ptr<void> > entities; 

	// list<unique_ptr<Device> > entities; 
	// list<unique_ptr<Channel> > entities; 
	// list<unique_ptr<ExecutingInviteCmd> > entities; 



	DataRestorerOperation()
	{
		operationType = OperationType::UNKNOWN_OPERATION;
		entityType = EntityType::UNKNOWN_ENTITY;
		gbDownlinkerDeviceId = "";
		workerThreadIdx = -1;
		deviceId = "";
		channelDeviceId = "";
		channelChannelDeviceId = "";
		inviteCmdSenderId = "";
		inviteDeviceId = "";
		inviteCmdSeq = 0;
		entities.clear();
	}

	DataRestorerOperation& operator=(const DataRestorerOperation& op)
	{
		operationType = op.operationType;
		entityType = op.entityType;
		gbDownlinkerDeviceId = op.gbDownlinkerDeviceId;
		workerThreadIdx = op.workerThreadIdx;
		deviceId = op.deviceId;
		channelDeviceId = op.channelDeviceId;
		channelChannelDeviceId = op.channelChannelDeviceId;
		inviteCmdSenderId = op.inviteCmdSenderId;
		inviteDeviceId = op.inviteDeviceId;
		inviteCmdSeq = op.inviteCmdSeq;

		// *this now responsible for op.entities
		for (std::list<void*>::const_iterator it = op.entities.begin(); it != op.entities.end(); ++it)
		{
			entities.push_back(*it);
		}
		return *this;
	}
};

} // namespace GBDownLinker
#endif
