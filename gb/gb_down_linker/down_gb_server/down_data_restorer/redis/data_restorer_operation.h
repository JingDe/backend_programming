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
	
Device* CopyDevice(Device* d);
Channel* CopyChannel(Channel* c);
ExecutingInviteCmd* CopyExecutingInviteCmd(ExecutingInviteCmd* eic);


struct DataRestorerOperation{
	enum OperationType{
		UNKNOWN=0,
		INSERT,
		UPDATE,
		DELETE,
		CLEAR,
	} operation_type;

	enum EntityType{
		UNKNOWN=0,
		DEVICE,
		CHANNEL,
		EXECUTING_INVITE_CMD,
	} entity_type;
											
	std::string gbdownlinker_device_id;
	int worker_thread_idx;

	std::string entity_id;
	std::string invite_cmd_sender_id;
	std::string invite_device_id;
	int64_t invite_cmd_seq;

	
	std::list<void*> entities; // pointer released in DownDataRestorer, after command sent

	// list<shared_ptr<void> > entities; 

	// list<unique_ptr<Device> > entities; 
	// list<unique_ptr<Channel> > entities; 
	// list<unique_ptr<ExecutingInviteCmd> > entities; 

    

    DataRestorerOperation()
    {
		operation_type = UNKNOWN;
		entity_type = UNKNOWN;
		gbdown_linker_device_id = "";
		worker_thread_idx = -1;
		entity_id = "";
		invite_cmd_sender_id = "";
		invite_device_id = "";
		invite_cmd_seq = "";
		entities.clear();
	}
    
	//DataRestorerOperation(std::string gbdownlinker_device_id, OperationType operation_type, EntityType entity_type, void* entity, int worker_thread_idx)
	//{
	//	gbdownlinker_device_id = gbdownlinker_device_id;
	//	gbdownlinker_device_id=operation_type;
	//	entity_type=entity_type;
	//	worker_thread_idx=worker_thread_idx;
	//	
	//	if(operation_type==INSERT  ||  operation_type==DELETE)
	//	{
	//		switch(entity_type)
	//		{
	//		case DEVICE:
	//			{
 //               Device* device=(Device*)entity;
	//			LOG_WRITE_INFO("to copy Device");
	//			// void* new_entity=(void*)(new Device(*device));
	//			void* new_entity=DeviceCopy(device);				
	//			LOG_WRITE_INFO("copy device ok");	
	//			// std::unique_ptr<Device> device_ptr = device->Clone();
	//			// new_entity=(void*)device_ptr.get();
	//			
	//			entities.push_back(new_entity);
 //               }
	//			break;
	//		case CHANNEL:
	//			{
 //               Channel* channel=(Channel*)entity;
	//			// void* new_entity=(void*)(new Channel(*channel));
	//			void* new_entity=(void*)(ChannelCopy(channel));
	//			
	//			// std::unique_ptr<Channel> new_entity = channel->Clone();
	//			
	//			entities.push_back(new_entity);
	//			}
 //               break;
 //           case EXECUTING_INVITE_CMD:
	//			{
 //               ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)entity;
	//			// void* new_entity=(void*)(new ExecutingInviteCmd(*cmd));
	//			void* new_entity=(void*)(ExecutingInviteCmdCopy(cmd));
	//			
	//			// std::unique_ptr<ExecutingInviteCmd> new_entity = cmd->Clone();
	//			
	//			entities.push_back(new_entity);
	//			}
 //               break;
 //           default:
	//			assert(false);
	//		}
	//	}
	//	else if(operation_type==CLEAR)
	//	{
	//		
	//	}
	//	else
	//	{
	//		assert(false);
	//	}
	//}
	//
	//DataRestorerOperation(std::string gbdownlinker_device_id, OperationType operation_type, EntityType entity_type, const list<void*>& entities, int worker_thread_idx)
	//{
	//	gbdownlinker_device_id = gbdownlinker_device_id;
	//	gbdownlinker_device_id=operation_type;
	//	entity_type=entity_type;
	//	worker_thread_idx=worker_thread_idx;
	//	
	//	if(operation_type==UPDATE)
	//	{
	//		switch(entity_type)
	//		{
	//		case DEVICE:
	//			for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
	//			{
	//				Device* device=(Device*)(*it);
	//				// void* new_entity=(void*)(new Device(*device));
	//				void* new_entity=(void*)(DeviceCopy(device));	
	//				
	//				// std::unique_ptr<Device> new_entity = device->Clone();
	//				entities.push_back(new_entity);
	//			}
	//			break;
	//		case CHANNEL:
	//			for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
	//			{
	//				Channel* channel=(Channel*)(*it);
	//				// void* new_entity=(void*)(new Channel(*channel));
	//				void* new_entity=(void*)(ChannelCopy(channel));

	//				// std::unique_ptr<Channel> new_entity = channel->Clone();
	//				
	//				entities.push_back(new_entity);
	//			}
	//			break;
	//		case EXECUTING_INVITE_CMD:
	//			for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
	//			{
	//				ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)(*it);
	//				// void* new_entity=(void*)(new ExecutingInviteCmd(*cmd));
	//				void* new_entity=(void*)(ExecutingInviteCmdCopy(cmd));
	//				
	//				// std::unique_ptr<ExecutingInviteCmd> new_entity = cmd->Clone();
	//				
	//				entities.push_back(new_entity);
	//			}
	//			break;
	//		default:
	//			assert(false);
	//		}
	//	}
	//	else
	//	{
	//		assert(false);
	//	}
	//}



	DataRestorerOperation& operator=(const DataRestorerOperation& op)
	{
		operation_type = op.operation_type;
		entity_type=op.entity_type;
		gbdownlinker_device_id = op.gbdownlinker_device_id;
		worker_thread_idx = op.worker_thread_idx;
		entity_id = op.entity_id;
		invite_cmd_sender_id = op.invite_cmd_sender_id;
		invite_device_id = op.invite_device_id;
		invite_cmd_seq = op.invite_cmd_seq;
		
		entities.swap(op.entities);
        //for(list<void*>::const_iterator it=op.entities.begin(); it!=op.entities.end(); ++it)
        //{
        //    entities.push_back(*it);
        //}
        return *this;
	}
};


} // namespace GBGateway
#endif
