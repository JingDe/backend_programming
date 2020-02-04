#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_H

#include"downdatarestorerdef.h"
#include"device.h"
#include"channel.h"
#include"executinginvitecmd.h"
#include"mutexlock.h"
#include"condvar.h"

#include<vector>
#include<list>
#include<queue>
#include<pthread.h>

using std::vector;
using std::list;
using std::queue;

#define DDR_OK 		0
#define DDR_FAIL 	-1

class RedisClient;
class DeviceMgr;
class ChannelMgr;
class ExecutingInviteCmdMgr;
class CountDownLatch;

struct DataRestorerOperation{
	enum OperationType{
		INSERT,
		DELETE,
		CLEAR,
		UPDATE,
	} operation_type_;
	enum EntityType{
		DEVICE,
		CHANNEL,
		EXECUTING_INVITE_CMD,
	} entity_type_;
	list<void*> entities_; // for operation type INSERT, DELELTE, for UPDATE
	int executing_invite_cmd_list_num_; // for entity type EXECUTING_INVITE_CMD
	// TODO 
	// vector<ExecutingInviteCmdList> entity_lists;
    
    DataRestorerOperation()
    {}
	DataRestorerOperation(OperationType operation_type, EntityType entity_type, void* entity)
	{
		operation_type_=operation_type;
		entity_type_=entity_type;
        void* entity_=0;

		if(operation_type==INSERT  ||  operation_type==DELETE)
		{
			switch(entity_type)
			{
			case DEVICE:
				{
                Device* device=(Device*)entity;
				entity_=(void*)(new Device(*device));
				entities_.push_back(entity_);
                }
				break;
			case CHANNEL:
				{
                Channel* channel=(Channel*)entity;
				entity_=(void*)(new Channel(*channel));
				entities_.push_back(entity_);
				}
                break;				
			}
		}
		else if(operation_type==CLEAR)
		{}		
	}
	DataRestorerOperation(OperationType operation_type, EntityType entity_type, void* entity, int worker_thread_num)
	{
		operation_type_=operation_type;
		entity_type_=entity_type;
		executing_invite_cmd_list_num_=worker_thread_num;

		if(operation_type==INSERT  ||  operation_type==DELETE)
		{
			switch(entity_type)
			{
			case EXECUTING_INVITE_CMD:
				{
                ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)entity;
				void* entity_=(void*)(new ExecutingInviteCmd(*cmd));
				entities_.push_back(entity_);
				}
                break;
			}
		}	
	}
	
	DataRestorerOperation(OperationType operation_type, EntityType entity_type, const list<void*>& entities)
	{
		operation_type_=operation_type;
		entity_type_=entity_type;
		if(operation_type==UPDATE)
		{
			switch(entity_type)
			{
			case DEVICE:
				for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
				{
					Device* device=(Device*)(*it);
					void* entity_=(void*)(new Device(*device));
					entities_.push_back(entity_);
				}
				break;
			case CHANNEL:
				for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
				{
					Channel* channel=(Channel*)(*it);
					void* entity_=(void*)(new Channel(*channel));
					entities_.push_back(entity_);
				}
				break;
			case EXECUTING_INVITE_CMD:
				for(list<void*>::const_iterator it=entities.begin(); it!=entities.end(); ++it)
				{
					ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)(*it);
					void* entity_=(void*)(new ExecutingInviteCmd(*cmd));
					entities_.push_back(entity_);
				}
				break;
			}
		}
	}

	DataRestorerOperation& operator=(const DataRestorerOperation& op)
	{
		operation_type_=op.operation_type_;
		entity_type_=op.entity_type_;
		executing_invite_cmd_list_num_=op.executing_invite_cmd_list_num_;
//		entities_.swap(op.entities_);
        for(list<void*>::const_iterator it=op.entities_.begin(); it!=op.entities_.end(); ++it)
        {
            entities_.push_back(*it);
        }
        return *this;
	}
};

class DownDataRestorer{
//	typedef list<ExecutingInviteCmd> ExecutingInviteCmdList;

public:
    DownDataRestorer();
	~DownDataRestorer();
	
    int Init(string redis_server_ip, uint16_t redis_server_port, int connection_num);
    int Uninit();
    int Start();
    int Stop();
    int GetStat();

	int LoadDeviceList(list<Device>& devices);
	int SelectDeviceList(Device& device);
	int InsertDeviceList(const Device& device);
	int DeleteDeviceList(const Device& device);
	int ClearDeviceList();
	int UpdateDeviceList(const list<Device>& devices);

	int LoadChannelList(list<Channel>& channels);
	int SelectChannelList(Channel& channel);
	int InsertChannelList(const Channel& channel);
	int DeleteChannelList(const Channel& channel);
	int ClearChannelList();
	int UpdateChannelList(const list<Channel>& channels);

	int LoadExecutingInviteCmdList(vector<ExecutingInviteCmdList>& executinginvitecmdlists);
	int SelectExecutingInviteCmdList(); // TODO
	int InsertExecutingInviteCmdList(const ExecutingInviteCmd& executinginvitecmd, int);
	int DeleteExecutingInviteCmdList(const ExecutingInviteCmd& executinginvitecmd, int);
	int ClearExecutingInviteCmdList();
	// TODO
	int UpdateExecutingInviteCmdList(const ExecutingInviteCmdList& executinginvitecmdlist);
	int UpdateExecutingInviteCmdList(const vector<ExecutingInviteCmdList>& executinginvitecmdlists);

private:
    static void* DataRestorerThreadFuncWrapper(void* arg);
    void DataRestorerThreadFunc();
	int GetWorkerThreadNum();
	
	RedisClient* redis_client_;
	bool inited_;
	bool started_;
    bool force_thread_exit_;	
	int data_restorer_threads_num_; // same as RedisClient m_connectionNum
	vector<pthread_t> data_restorer_threads_;
	CountDownLatch* data_restorer_threads_exit_latch;
	
	DeviceMgr* device_mgr_;
	ChannelMgr* channel_mgr_;
	ExecutingInviteCmdMgr* executing_invite_cmd_mgr_;

	MutexLock operations_queue_mutex_;
	CondVar operations_queue_not_empty_condvar;
	queue<DataRestorerOperation> operations_queue_;
};

#endif
