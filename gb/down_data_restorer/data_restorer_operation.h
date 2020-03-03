#ifndef DOWN_DATA_RESTORER_OPERATION_H
#define DOWN_DATA_RESTORER_OPERATION_H

#include"channel.h"
#include"device.h"
#include"executing_invite_cmd.h"

#include<cassert>
#include<list>
using std::list;

namespace GBGateway {
	
Device* DeviceCopy(Device* d);
Channel* ChannelCopy(Channel* c);
ExecutingInviteCmd* ExecutingInviteCmdCopy(ExecutingInviteCmd* eic);


struct DataRestorerOperation{
	enum OperationType{
		INSERT=0,
		DELETE,
		CLEAR,
		UPDATE,
	} operation_type_;
	enum EntityType{
		DEVICE=0,
		CHANNEL,
		EXECUTING_INVITE_CMD,
	} entity_type_;
	
	// pointer released in DownDataRestorer, after command sent
	list<void*> entities_; // for operation type INSERT, DELELTE, for UPDATE
	// list<unique_ptr<void> > entities_; 
	// list<shared_ptr<void> > entities_; 
	int executing_invite_cmd_list_no_; // for entity type EXECUTING_INVITE_CMD
	
	// vector<ExecutingInviteCmdList> entity_lists;
    
    DataRestorerOperation()
    {}
    
	DataRestorerOperation(OperationType operation_type, EntityType entity_type, void* entity, int worker_thread_no)
	{
		operation_type_=operation_type;
		entity_type_=entity_type;
		executing_invite_cmd_list_no_=worker_thread_no;
		
		if(operation_type==INSERT  ||  operation_type==DELETE)
		{
			switch(entity_type)
			{
			case DEVICE:
				{
                Device* device=(Device*)entity;
				// void* new_entity=(void*)(new Device(*device));
				void* new_entity=DeviceCopy(device);				
				
				// std::unique_ptr<Device> device_ptr = device->Clone();
				// new_entity=(void*)device_ptr.get();
				
				entities_.push_back(new_entity);
                }
				break;
			case CHANNEL:
				{
                Channel* channel=(Channel*)entity;
				// void* new_entity=(void*)(new Channel(*channel));
				void* new_entity=(void*)(ChannelCopy(channel));
				
				// std::unique_ptr<Channel> new_entity = channel->Clone();
				
				entities_.push_back(new_entity);
				}
                break;
            case EXECUTING_INVITE_CMD:
				{
                ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)entity;
				// void* new_entity=(void*)(new ExecutingInviteCmd(*cmd));
				void* new_entity=(void*)(ExecutingInviteCmdCopy(cmd));
				
				// std::unique_ptr<ExecutingInviteCmd> new_entity = cmd->Clone();
				
				entities_.push_back(new_entity);
				}
                break;
            default:
				assert(false);
			}
		}
		else if(operation_type==CLEAR)
		{
			
		}
		else
		{
			assert(false);
		}
	}
	
	DataRestorerOperation(OperationType operation_type, EntityType entity_type, const list<void*>& entities, int worker_thread_no)
	{
		operation_type_=operation_type;
		entity_type_=entity_type;
		executing_invite_cmd_list_no_=worker_thread_no;
		
		if(operation_type==UPDATE)
		{
			switch(entity_type)
			{
			case DEVICE:
				for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
				{
					Device* device=(Device*)(*it);
					// void* new_entity=(void*)(new Device(*device));
					void* new_entity=(void*)(DeviceCopy(device));	
					
					// std::unique_ptr<Device> new_entity = device->Clone();
					entities_.push_back(new_entity);
				}
				break;
			case CHANNEL:
				for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
				{
					Channel* channel=(Channel*)(*it);
					// void* new_entity=(void*)(new Channel(*channel));
					void* new_entity=(void*)(ChannelCopy(channel));

					// std::unique_ptr<Channel> new_entity = channel->Clone();
					
					entities_.push_back(new_entity);
				}
				break;
			case EXECUTING_INVITE_CMD:
				for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
				{
					ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)(*it);
					// void* new_entity=(void*)(new ExecutingInviteCmd(*cmd));
					void* new_entity=(void*)(ExecutingInviteCmdCopy(cmd));
					
					// std::unique_ptr<ExecutingInviteCmd> new_entity = cmd->Clone();
					
					entities_.push_back(new_entity);
				}
				break;
			default:
				assert(false);
			}
		}
		else
		{
			assert(false);
		}
	}

	DataRestorerOperation& operator=(const DataRestorerOperation& op)
	{
		operation_type_=op.operation_type_;
		entity_type_=op.entity_type_;
		executing_invite_cmd_list_no_=op.executing_invite_cmd_list_no_;
		
//		entities_.swap(op.entities_);
        for(list<void*>::const_iterator it=op.entities_.begin(); it!=op.entities_.end(); ++it)
        {
            entities_.push_back(*it);
        }
		
		// for(list<unique_ptr<void> >::iterator it=op.entities_.begin(); it!=op.entities_.end(); ++it)
		// {
			// // entities_.push_back(it->Clone());
			// entities_.push_back(static_cast<unique_ptr<void> >(it->Clone()));
		// }
        return *this;
	}
};

} // namespace GBGateway
#endif
