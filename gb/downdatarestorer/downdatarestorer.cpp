#include"downdatarestorer.h"
#include"countdownlatch.h"
#include"devicemgr.h"
#include"channelmgr.h"
#include"executinginvitecmdmgr.h"
#include"mutexlockguard.h"
#include"redisclient.h"

#include<cassert>

#include"glog/logging.h"

DownDataRestorer::DownDataRestorer()
	:redis_client_(NULL),
	inited_(false),
	started_(false),
    force_thread_exit_(false),
	data_restorer_background_threads_num_(0),
	data_restorer_threads_(),
	data_restorer_threads_exit_latch(NULL),
	device_mgr_(NULL),
	channel_mgr_(NULL),
	executing_invite_cmd_mgr_(NULL),
	operations_queue_mutex_(),
	operations_queue_not_empty_condvar(&operations_queue_mutex_),
	operations_queue_(),
	worker_thread_num_(0),
	redis_mode_(STAND_ALONE_OR_PROXY_MODE)
{
//	google::InitGoogleLogging("downdatarestorer");
//	FLAGS_logtostderr=false;
//	FLAGS_log_dir="./downdatarestorerlogdir";
//	FLAGS_alsologtostderr=true;
//	FLAGS_colorlogtostderr=true;
	
	LOG(INFO)<<"DownDataRestorer constructed ok";
}

DownDataRestorer::~DownDataRestorer()
{
	LOG(INFO)<<"DownDataRestorer descontruction";

	if(started_)
		Stop();
	if(inited_)
		Uninit();
		
//	google::ShutdownGoogleLogging();	
}

int DownDataRestorer::Init(const string& redis_server_ip, uint16_t redis_server_port, int worker_thread_num)
{
	if(inited_)
	{
		LOG(WARNING)<<"init called repeatedly";
		return DDR_FAIL;
	}
	
	LOG(INFO)<<"DownDataRestorer init now, redis ip "<<redis_server_ip<<", redis port "<<redis_server_port<<", worker_thread num "<<worker_thread_num;
	RedisServerInfo redis_server(redis_server_ip, redis_server_port);
	REDIS_SERVER_LIST redis_server_list({redis_server});

	return Init(redis_server_list, worker_thread_num);
}

int DownDataRestorer::Init(const REDIS_SERVER_LIST& redis_server_list, int worker_thread_num)
{
	if(inited_)
	{
		LOG(WARNING)<<"init called repeatedly";
		return DDR_FAIL;
	}

	worker_thread_num_=worker_thread_num;
	
	if(redis_server_list.size()==1)
	{
		redis_mode_=STAND_ALONE_OR_PROXY_MODE;
	}
	else
	{
		redis_mode_=CLUSTER_MODE;
	}
	LOG_IF(INFO, redis_mode_==STAND_ALONE_OR_PROXY_MODE)<<"redis mode is STAND_ALONE_OR_PROXY_MODE";
	LOG_IF(INFO, redis_mode_==CLUSTER_MODE)<<"redis mode is CLUSTER_MODE";
	LOG_IF(INFO, redis_mode_==SENTINEL_MODE)<<"redis mode is SENTINEL_MODE";

	assert(redis_client_=NULL);
	redis_client_=new RedisClient();
	if(redis_client_==NULL)
	{
		LOG(ERROR)<<"create RedisClient failed";
		return DDR_FAIL;
	}

	// connection_num RedisConnection, for worker_thread_num workers AND 1 background_thread
	data_restorer_background_threads_num_=1; 
	int connection_num=worker_thread_num_+data_restorer_background_threads_num_;
	LOG(INFO)<<"background_restorer_thread num is "<<data_restorer_background_threads_num_;
	LOG(INFO)<<"worker_thead num is "<<worker_thread_num_;
	LOG(INFO)<<"redis_connection num is "<<connection_num;
//	uint32_t keepalive_time_secs=86400;
	if(redis_client_->init(redis_server_list, connection_num/*, keepalive_time_secs*/)==false)
	{
		LOG(ERROR)<<"init RedisClient failed";
		delete redis_client_;
		redis_client_=NULL;
		return DDR_FAIL;
	}
	
	inited_=true;
	return DDR_OK;
}

int DownDataRestorer::Uninit()
{
	if(inited_  &&  redis_client_)
	{
		redis_client_->freeRedisClient();
		delete redis_client_;
		redis_client_=NULL;
	}
	LOG(INFO)<<"DownDataRestorer Uninit now";
	inited_=false;
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
	LOG(INFO)<<"DownDataRestorer Start now";

	device_mgr_=new DeviceMgr(redis_client_, redis_mode_);
	channel_mgr_=new ChannelMgr(redis_client_, redis_mode_);
	executing_invite_cmd_mgr_=new ExecutingInviteCmdMgr(redis_client_, redis_mode_, worker_thread_num_);
	if(!device_mgr_  ||  !channel_mgr_	||	!executing_invite_cmd_mgr_)
	{
		if(device_mgr_)
		{
			delete device_mgr_;
			device_mgr_=NULL;
		}
		if(channel_mgr_)
		{
			delete channel_mgr_;
			channel_mgr_=NULL;
		}
		if(executing_invite_cmd_mgr_)
		{
			delete executing_invite_cmd_mgr_;
			executing_invite_cmd_mgr_=NULL;
		}
		return DDR_FAIL;
	}

	
	data_restorer_threads_.reserve(data_restorer_background_threads_num_);
	int ret=0;
	pthread_t thread_id;
	for(int i=0; i<data_restorer_background_threads_num_; i++)
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
	//data_restorer_threads_.shrink_to_fit();
    data_restorer_threads_.resize(data_restorer_threads_.size());
    
//    assert(data_restorer_threads_exit_latch==NULL);
//	if(data_restorer_threads_exit_latch)
//		delete data_restorer_threads_exit_latch;
	data_restorer_threads_exit_latch=new CountDownLatch(data_restorer_threads_.size());
	if(data_restorer_threads_exit_latch==NULL)
	{
		LOG(ERROR)<<"new data_restorer_threads_exit_latch failed";
		return DDR_FAIL;
	}
	if(data_restorer_threads_exit_latch->IsValid()==false)
	{
		LOG(ERROR)<<"init CountDownLatch failed";
		delete data_restorer_threads_exit_latch;
		data_restorer_threads_exit_latch=NULL;

		KillDataRestorerThread();
		
		return DDR_FAIL;
	}
	
	started_=true;
	return DDR_OK;
}

int DownDataRestorer::GetWorkerThreadNum()
{
	return worker_thread_num_;
}

int DownDataRestorer::Stop()
{
	if(!started_)
	{
		LOG(WARNING)<<"not started yet";
		return DDR_FAIL;
	}
	LOG(INFO)<<"DownDataRestorer stop now";

	force_thread_exit_=true;
	operations_queue_not_empty_condvar.SignalAll();
	data_restorer_threads_exit_latch->Wait();
	
	delete data_restorer_threads_exit_latch;
	data_restorer_threads_exit_latch=NULL;

	if(device_mgr_)
	{
		delete device_mgr_;
		device_mgr_=NULL;
	}
	if(channel_mgr_)
	{
		delete channel_mgr_;
		channel_mgr_=NULL;
	}
	if(executing_invite_cmd_mgr_)
	{
		delete executing_invite_cmd_mgr_;
		executing_invite_cmd_mgr_=NULL;
	}
	
	started_=false;
	return DDR_OK;
}

void DownDataRestorer::KillDataRestorerThread()
{
	for(size_t i=0; i<data_restorer_threads_.size(); i++)
	{
		pthread_cancel(data_restorer_threads_[i]);
	}
}

void* DownDataRestorer::DataRestorerThreadFuncWrapper(void* arg)
{
	DownDataRestorer* down_data_restorer=(DownDataRestorer*)arg;
	down_data_restorer->DataRestorerThreadFunc();
	return 0;
}

void DownDataRestorer::DataRestorerThreadFunc()
{
	LOG(INFO)<<"restorer thread started";
	
	while(!force_thread_exit_)
	{
		DataRestorerOperation operation;
		{
			MutexLockGuard gurad(&operations_queue_mutex_);
			while(operations_queue_.empty())
			{
				if(force_thread_exit_)
					break;
				operations_queue_not_empty_condvar.Wait();
			}
			if(force_thread_exit_)
				break;
			assert(!operations_queue_.empty());
			operation=operations_queue_.front();
			operations_queue_.pop();
		}

		if(operation.operation_type_==DataRestorerOperation::INSERT)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_->InsertDevice(*(Device*)(*operation.entities_.begin()));
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->InsertChannel(*(Channel*)(*(operation.entities_.begin())));
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Insert(*(ExecutingInviteCmd*)(*operation.entities_.begin()), operation.executing_invite_cmd_list_no_);
			}
		}
		else if(operation.operation_type_==DataRestorerOperation::DELETE)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_->DeleteDevice(*(Device*)(*operation.entities_.begin()));
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->DeleteChannel(*(Channel*)(*operation.entities_.begin()));
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Delete(*(ExecutingInviteCmd*)(*operation.entities_.begin()), operation.executing_invite_cmd_list_no_);
			}
		}
		else if(operation.operation_type_==DataRestorerOperation::CLEAR)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_->ClearDevices();
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->ClearChannels();
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				if(operation.executing_invite_cmd_list_no_==-1)
					executing_invite_cmd_mgr_->Clear();
				else
					executing_invite_cmd_mgr_->Clear(operation.executing_invite_cmd_list_no_);
			}
		}
		else if(operation.operation_type_==DataRestorerOperation::UPDATE)
		{
			if(operation.entity_type_==DataRestorerOperation::DEVICE)
			{
				device_mgr_->UpdateDevices(operation.entities_);
			}
			else if(operation.entity_type_==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->UpdateChannels(operation.entities_);
			}
			else if(operation.entity_type_==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Update(operation.entities_, operation.executing_invite_cmd_list_no_);
			}
		}
		
		for(list<void*>::iterator it=operation.entities_.begin(); it!=operation.entities_.end(); it++)
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

	data_restorer_threads_exit_latch->CountDown();
	LOG(INFO)<<"data restorer thread exits now";
}

// TODO
int DownDataRestorer::GetStat()
{	
	LOG(INFO)<<"DownDataRestorer stat: ";
	LOG(INFO)<<"device count: "<<GetDeviceCount();
	LOG(INFO)<<"channel count: "<<GetChannelCount();
	int worker_thread_num=GetWorkerThreadNum();
	for(int i=0; i<worker_thread_num; i++)
	{
		LOG(INFO)<<"ExecutingInviteCmdList["<<i<<"] count: "<<GetExecutingInviteCmdCount(i);
	}
	return DDR_OK;
}

int DownDataRestorer::GetDeviceCount()
{
	return device_mgr_->GetDeviceCount();
}

int DownDataRestorer::GetChannelCount()
{
	return channel_mgr_->GetChannelCount();
}

int DownDataRestorer::GetExecutingInviteCmdCount(int worker_thread_no)
{
	return executing_invite_cmd_mgr_->GetExecutingInviteCmdListSize(worker_thread_no);
}

int DownDataRestorer::LoadDeviceList(list<Device>& devices)
{
	assert(started_);
	device_mgr_->LoadDevices(devices);
	return DDR_OK;
}

int DownDataRestorer::SelectDeviceList(const string& device_id, Device& device)
{
	assert(started_);
	if(device_mgr_->SearchDevice(device_id, device)<0)
		return DDR_FAIL;
	return DDR_OK;
}

int DownDataRestorer::InsertDeviceList(const Device& device)
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::INSERT, DataRestorerOperation::DEVICE, const_cast<Device*>(&device), -1);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::DeleteDeviceList(const Device& device)
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::DELETE, DataRestorerOperation::DEVICE, const_cast<Device*>(&device), -1);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::ClearDeviceList()
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::CLEAR, DataRestorerOperation::DEVICE, NULL, -1);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::UpdateDeviceList(const list<Device>& devices)
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	list<void*> device_list;
	for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		device_list.push_back(const_cast<Device*>(&(*it)));
	}
	DataRestorerOperation operation(DataRestorerOperation::UPDATE, DataRestorerOperation::DEVICE, device_list, -1);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::LoadChannelList(list<Channel>& channels)
{
    assert(started_);
    channel_mgr_->LoadChannels(channels);
    return DDR_OK;
}

int DownDataRestorer::SelectChannelList(const string& channel_id, Channel& channel)
{
    assert(started_);
    if(channel_mgr_->SearchChannel(channel_id, channel)<0)
        return DDR_FAIL;
    return DDR_OK;
}

int DownDataRestorer::InsertChannelList(const Channel& channel)
{
    assert(started_);
    MutexLockGuard guard(&operations_queue_mutex_);
    DataRestorerOperation operation(DataRestorerOperation::INSERT, DataRestorerOperation::CHANNEL, const_cast<Channel*>(&channel), -1);
    operations_queue_.push(operation);
    operations_queue_not_empty_condvar.SignalAll();
    return DDR_OK;
}

int DownDataRestorer::DeleteChannelList(const Channel& channel)
{
    assert(started_);
    MutexLockGuard guard(&operations_queue_mutex_);
    DataRestorerOperation operation(DataRestorerOperation::DELETE, DataRestorerOperation::CHANNEL, const_cast<Channel*>(&channel), -1);
    operations_queue_.push(operation);
    operations_queue_not_empty_condvar.SignalAll();
    return DDR_OK;
}

int DownDataRestorer::ClearChannelList()
{
    assert(started_);
    MutexLockGuard guard(&operations_queue_mutex_);
    DataRestorerOperation operation(DataRestorerOperation::CLEAR, DataRestorerOperation::CHANNEL, NULL, -1);
    operations_queue_.push(operation);
    operations_queue_not_empty_condvar.SignalAll();
    return DDR_OK;
}

int DownDataRestorer::UpdateChannelList(const list<Channel>& channels)
{
    assert(started_);
    MutexLockGuard guard(&operations_queue_mutex_);
    list<void*> channel_list;
    for(list<Channel>::const_iterator it=channels.begin(); it!=channels.end(); ++it)
    {
        channel_list.push_back(const_cast<Channel*>(&(*it)));
    }
    DataRestorerOperation operation(DataRestorerOperation::UPDATE, DataRestorerOperation::CHANNEL, channel_list, -1);
    operations_queue_.push(operation);
    operations_queue_not_empty_condvar.SignalAll();
    return DDR_OK;
}

int DownDataRestorer::LoadExecutingInviteCmdList(vector<ExecutingInviteCmdList>& executing_invite_cmd_lists)
{
	assert(started_);
	executing_invite_cmd_mgr_->Load(executing_invite_cmd_lists);
	return DDR_OK;
}

int DownDataRestorer::LoadExecutingInviteCmdList(int worker_thread_no, ExecutingInviteCmdList& executing_invite_cmd_list)
{
	assert(started_);
	executing_invite_cmd_mgr_->Load(worker_thread_no, executing_invite_cmd_list);
	return DDR_OK;
}

int DownDataRestorer::SelectExecutingInviteCmdList(int worker_thread_no, const string& cmd_id, ExecutingInviteCmd& cmd)
{
	assert(started_);
	if(executing_invite_cmd_mgr_->Search(cmd_id, cmd, worker_thread_no)<0)
		return DDR_FAIL;
	return DDR_OK;
}

int DownDataRestorer::InsertExecutingInviteCmdList(int worker_thread_no, const ExecutingInviteCmd& cmd)
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::INSERT, DataRestorerOperation::EXECUTING_INVITE_CMD, const_cast<ExecutingInviteCmd*>(&cmd), worker_thread_no);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.Signal();
	return DDR_OK;
}

int DownDataRestorer::DeleteExecutingInviteCmdList(int worker_thread_no, const ExecutingInviteCmd& executinginvitecmd)
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::DELETE, DataRestorerOperation::EXECUTING_INVITE_CMD, const_cast<ExecutingInviteCmd*>(&executinginvitecmd), worker_thread_no);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::ClearExecutingInviteCmdList()
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::CLEAR, DataRestorerOperation::EXECUTING_INVITE_CMD, NULL, -1);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::ClearExecutingInviteCmdList(int worker_thread_no)
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	DataRestorerOperation operation(DataRestorerOperation::CLEAR, DataRestorerOperation::EXECUTING_INVITE_CMD, NULL, worker_thread_no);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}


int DownDataRestorer::UpdateExecutingInviteCmdList(int worker_thread_no, const ExecutingInviteCmdList& executing_invite_cmd_list)
{
	assert(started_);
	MutexLockGuard guard(&operations_queue_mutex_);
	list<void*> cmd_list;
	for(ExecutingInviteCmdList::const_iterator it=executing_invite_cmd_list.begin(); it!=executing_invite_cmd_list.end(); ++it)
	{
		cmd_list.push_back(const_cast<ExecutingInviteCmd*>(&(*it)));
	}
	DataRestorerOperation operation(DataRestorerOperation::UPDATE, DataRestorerOperation::EXECUTING_INVITE_CMD, cmd_list, worker_thread_no);
	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return DDR_OK;
}

int DownDataRestorer::UpdateExecutingInviteCmdList(const vector<ExecutingInviteCmdList>& executing_invite_cmd_lists)
{
	assert(started_);
	assert((int)executing_invite_cmd_lists.size()==GetWorkerThreadNum());
	for(size_t i=0; i<executing_invite_cmd_lists.size(); ++i)
	{
		UpdateExecutingInviteCmdList(i, executing_invite_cmd_lists[i]);
	}
	return DDR_OK;
}



