#include"downdatastorer.h"
#include"countdownlatch.h"
#include"devicemgr.h"
#include"channelmgr.h"
#include"executinginvitecmdmgr.h"
#include"glog/logging.h"

DownDataRestorer::DownDataRestorer()
	:redis_client_(NULL),
	inited_(false),
	started_(false),
	data_restorer_threads_num_(0),
	data_restorer_threads_(),
	data_restorer_threads_exit_latch(NULL),
	operations_queue_mutex_(),
	operations_queue_not_empty_condvar(&operations_queue_mutex_),
	operations_queue_()
{
	google::InitGoogleLogging("downdatarestorer");
	FLAGS_logtostderr=false;
	FLAGS_log_dir="./downdatarestorerlogdir";
	FLAGS_alsologtostderr=true;
	FLAGS_colorlogtostderr=true;
	
	LOG(INFO)<<"DownDataRestorer constructed ok";
}

DownDataRestorer::~DownDataRestorer()
{
	LOG(INFO)<<"DownDataRestorer descontruction";

	if(started_)
		Stop();
	if(inited_)
		Uninit();
	if(device_mgr_)
		delete device_mgr_;
	if(channel_mgr_)
		delete channel_mgr_;
	if(executing_invite_cmd_mgr_)
		delete executing_invite_cmd_mgr_;
	google::ShutdownGoogleLogging();	
}

int DownDataRestorer::Init(string redis_server_ip, uint16_t redis_server_port, int connection_num)
{
	if(inited_)
	{
		LOG(WARNING)<<"init called repeatedly";
		return DDR_FAIL;
	}
	LOG(INFO)<<"DownDataRestorer init, redis ip "<<redis_server_ip<<", redis port"<<redis_server_port<<", connection num "<<connection_num;

	RedisServerInfo redis_server(redis_server_ip, redis_server_port);
	REDIS_SERVER_LIST redis_server_list({redis_server});
	data_restorer_threads_num_=connection_num;
	uint32_t keepalive_time_secs=86400;

	redis_client_=new RedisClient();
	if(redis_client_==NULL)
	{
		LOG(ERROR)<<"create RedisClient failed";
		return DDR_FAIL;
	}
	
	if(redis_client_.init(redis_server_list, connection_num, keepalive_time_secs)==false)
	{
		LOG(ERROR)<<"init RedisClient failed";
		return -1;
	}
	inited_=true;
	return DDR_OK;
}

int DownDataRestorer::Uninit()
{
	if(inited_  &&  redis_client_)
	{
		redis_client_.freeRedisClient();
		redis_client_=NULL;
	}
	return DDR_OK;
}

int DownDataRestorer::Start()
{
	if(!inited_)
	{
		LOG(WARNING)<<"please init DownDataRestorer before";
		return DDR_FAIL;
	}

	if(started_)
	{
		LOG(WARNING)<<"start called repeatedly";
		return DDR_FAIL;
	}

	data_restorer_threads_.reserve(data_restorer_threads_num_);
	int ret=0;
	pthread_t thread_id;
	for(int i=0; i<data_restorer_threads_num_; i++)
	{		
		ret=pthread_create(&thread_id, NULL, DataRestorerThreadFuncWrapper, this);
		if(ret!=0)
		{
			LOG(WARNING)<<"pthread_create failed";
		}
		else
		{
			data_restorer_threads_.push_back(thread_id);
		}
	}
	if(data_restorer_threads_.empty())
	{
		LOG(ERROR)<<"start redis threads failed";
		return DDR_FAIL;
	}
	data_restorer_threads_.shrink_to_fit();
	data_restorer_threads_exit_latch=new CountDownLatch(data_restorer_threads_.size());
	if(data_restorer_threads_exit_latch.IsValid()==false)
	{
		LOG(ERROR)<<"init CountDownLatch failed";
		return DDR_FAIL;
	}

	device_mgr_=new DeviceMgr(redis_client_);
	channel_mgr_=new channel_mgr_(redis_client_);
	executing_invite_cmd_mgr_=new ExecutingInviteCmdMgr(redis_client_);
	if(!device_mgr_  ||  !channel_mgr_  ||  !executing_invite_cmd_mgr_)
	{
		return DDR_FAIL;
	}
	
	started_=true;
	return DDR_OK;
}

int DownDataRestorer::Stop()
{
	if(!started_)
	{
		LOG(WARNING)<<"not started yet";
		retun DDR_FAIL;
	}

	force_thread_exit=true;
	data_restorer_threads_exit_latch.Wait();

	started_=false;
	return DDR_OK;
}

void* DownDataRestorer::DataRestorerThreadFuncWrapper(void* arg)
{
	DownDataRestorer* down_data_restorer=(DownDataRestorer*)arg;
	down_data_restorer->DataRestorerThreadFunc();
	return 0;
}

void DownDataRestorer::DataRestorerThreadFunc()
{
	while(!force_thread_exit)
	{
		DataRestorerOperation operation;
		{
		MutexLockGuard gurad(&operations_queue_mutex_);
		while(operations_queue_.empty())
			operations_queue_not_empty_condvar.Wait();
		operation=operations_queue_.begin();
		operations_queue_.pop_front();
		}

		if(operation.operation_type_==DataRestorerOperation::INSERT)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_.InsertDevice(*(Device*)(operation.entities.begin());
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_.InsertChannel(*(Channel*)(operation.entities.begin());
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				
			}
		}
		else if(operation.operation_type_==DataRestorerOperation::DELETE)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_.DeleteDevice(*(Device*)(operation.entities.begin());
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_.DeleteChannel(*(Channel*)(operation.entities.begin());
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				
			}
		}
		else if(operation.operation_type_==DataRestorerOperation::CLEAR)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_.ClearDevices();
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_.ClearChannels();
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				
			}
		}
		else if(operation.operation_type_==DataRestorerOperation::UPDATE)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_.UpdateDevices(*(Device*)(operation.entities.begin());
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_.UpdateChannels(*(Channel*)(operation.entities.begin());
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				
			}
		}

		
		for(list<void*>::iterator it=operation.entities.begin(); it!=operation.entities.end(); it++)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				delete (Device*)(*it);
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				delete (Channel*)(*it);
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				delete (ExecutingInviteCmd*)(*it);
			}
		}
	}

	LOG(INFO)<<"data restorer thread exits now";
	data_restorer_threads_exit_latch.CountDown();
}

int DownDataRestorer::GetStat()
{
	// TODO
	return DDR_OK;
}

int DownDataRestorer::LoadDeviceList(list<Device>& devices)
{
	assert(started_);
	device_mgr_->LoadDevices(devices);
	return DDR_OK;
}

int DownDataRestorer::SelectDeviceList(Device& device)
{
	// TODO
	return DDR_OK;
}

int DownDataRestorer::InsertDeviceList(const Device& device)
{
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::INSERT, DataRestorerOperation::DEVICE, &device);
	operations_queue_.insert(operation);
	if(operations_queue_.size()==1)
		operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::DeleteDeviceList(const Device& device)
{
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::DELETE, DataRestorerOperation::DEVICE, &device);
	operations_queue_.insert(operation);
	if(operations_queue_.size()==1)
		operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::ClearDeviceList()
{
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::CLEAR, DataRestorerOperation::DEVICE, NULL);
	operations_queue_.insert(operation);
	if(operations_queue_.size()==1)
		operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::UpdateDeviceList(const list<Device>& devices)
{
	MutexLockGuard guard(&operations_queue_mutex_);
	list<void*> device_list;
	for(list<Device>::iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		device_list.push_back(&(*it));
	}
	DataRestorerOperation operation(DataRestorerOperation::UPDATE, DataRestorerOperation::DEVICE, device_list);
	operations_queue_.insert(operation);
	if(operations_queue_.size()==1)
		operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

/*
int LoadChannelList(list<Channel>& channels);
int SelectChannelList(Channel& channel);
int InsertChannelList(const Channel& channel);
int DeleteChannelList(const Channel& channel);
int ClearChannelList();
int UpdateChannelList(const list<Channel>& channels);
*/

int DownDataRestorer::LoadExecutingInviteCmdList(vector<ExecutingInviteCmdList>& executinginvitecmdlists)
{
	
}

int DownDataRestorer::SelectExecutingInviteCmdList()
{}

int DownDataRestorer::InsertExecutingInviteCmdList(const ExecutingInviteCmd& executinginvitecmd)
{}

int DownDataRestorer::DeleteExecutingInviteCmdList(const ExecutingInviteCmd& executinginvitecmd)
{}

int DownDataRestorer::ClearExecutingInviteCmdList()
{

}

int DownDataRestorer::UpdateExecutingInviteCmdList(const ExecutingInviteCmdList& executinginvitecmdlist)
{}

int DownDataRestorer::UpdateExecutingInviteCmdList(const vector<ExecutingInviteCmdList>& executinginvitecmdlists)
{}




