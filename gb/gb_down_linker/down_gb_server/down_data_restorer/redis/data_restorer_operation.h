#ifndef DOWN_DATA_RESTORER_OPERATION_H
#define DOWN_DATA_RESTORER_OPERATION_H

#include"channel.h"
#include"device.h"
#include"executing_invite_cmd.h"
#include"base_library/log.h"

#include<cassert>
#include<list>
#include<string>


namespace GBGateway {

struct DataRestorerOperation;

bool PushEntity(DataRestorerOperation& operation, Device* device);
bool PushEntity(DataRestorerOperation& operation, Channel* channel);
bool PushEntity(DataRestorerOperation& operation, ExecutingInviteCmd* invite);
	
Device* CopyDevice(Device* d);
Channel* CopyChannel(Channel* c);
ExecutingInviteCmd* CopyExecutingInviteCmd(ExecutingInviteCmd* eic);


struct DataRestorerOperation{
	enum OperationType{
		UNKNOWN_OPERATION=0,
		INSERT,
		UPDATE,
		DELETE,
		DELETE_CHANNEL_BY_ID,
		DELETE_CHANNEL_BY_DEVICE_ID,
		CLEAR,
	} operation_type;

	enum EntityType{
		UNKNOWN_ENTITY=0,
		DEVICE,
		CHANNEL,
		EXECUTING_INVITE_CMD,
	} entity_type;
											
	std::string gbdownlinker_device_id;
	int worker_thread_idx;

	// Device
	std::string device_id;

	// Channel
	std::string channel_device_id;
	std::string channel_channel_device_id;

	// ExecutingInviteCmd
	std::string invite_cmd_sender_id;
	std::string invite_device_id;
	int64_t invite_cmd_seq;

	
	std::list<void*> entities; // pointer released in DownDataRestorer, after command processed

	// list<shared_ptr<void> > entities; 

	// list<unique_ptr<Device> > entities; 
	// list<unique_ptr<Channel> > entities; 
	// list<unique_ptr<ExecutingInviteCmd> > entities; 

    

    DataRestorerOperation()
    {
		operation_type = OperationType::UNKNOWN_OPERATION;
		entity_type = EntityType::UNKNOWN_ENTITY;
		gbdownlinker_device_id = "";
		worker_thread_idx = -1;
		device_id = "";
		channel_device_id="";
		channel_channel_device_id="";
		invite_cmd_sender_id = "";
		invite_device_id = "";
		invite_cmd_seq = 0;
		entities.clear();
	}

	DataRestorerOperation& operator=(const DataRestorerOperation& op)
	{
		operation_type = op.operation_type;
		entity_type=op.entity_type;
		gbdownlinker_device_id = op.gbdownlinker_device_id;
		worker_thread_idx = op.worker_thread_idx;
		device_id = op.device_id;
		channel_device_id = op.channel_device_id;
		channel_channel_device_id = op.channel_channel_device_id;
		invite_cmd_sender_id = op.invite_cmd_sender_id;
		invite_device_id = op.invite_device_id;
		invite_cmd_seq = op.invite_cmd_seq;
		
		//entities.swap(op.entities);
        for(std::list<void*>::const_iterator it=op.entities.begin(); it!=op.entities.end(); ++it)
        {
            entities.push_back(*it);
        }
		//LOG_WRITE_INFO("operator= DataRestorerOperation");
		
        return *this;
	}
};


} // namespace GBGateway
#endif
