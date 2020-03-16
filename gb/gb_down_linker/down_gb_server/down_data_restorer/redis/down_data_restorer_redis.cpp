#include"down_data_restorer_redis.h"
#include"device.h"
#include"channel.h"
#include"executing_invite_cmd.h"
#include"countdownlatch.h"
#include"device_mgr.h"
#include"channel_mgr.h"
#include"executing_invite_cmd_mgr.h"
#include"mutexlockguard.h"
#include"base_library/log.h"

#include<sstream>
#include<cassert>


namespace GBGateway {

#define CHECK_CONDITION(condition, err_msg) do{\
			if(!condition)\
			{\
				std::stringstream log_msg;\
				log_msg << err_msg;\
				LOG_WRITE_ERROR(log_msg.str());\
				return RESTORER_FAIL;\
			}\
		}while(0);


CDownDataRestorerRedis::CDownDataRestorerRedis()
	:redis_mode_(STAND_ALONE_OR_PROXY_MODE),
	redis_client_(NULL),
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
	operations_queue_()
{	
	LOG_WRITE_INFO("CDownDataRestorerRedis constructed ok");
}

CDownDataRestorerRedis::~CDownDataRestorerRedis()
{
	LOG_WRITE_INFO("CDownDataRestorerRedis descontruction");

	if(started_)
		Stop();
	if(inited_)
		Uninit();		
}

int CDownDataRestorerRedis::Init(RestorerParamPtr restorer_param/*, ErrorReportCallback error_callback*/)
{
	if(inited_)
	{
		LOG_WRITE_WARNING("init called repeatedly");
		return RESTORER_FAIL;
	}

	assert(restorer_param->type== RestorerParam::Type::Redis);

	std::stringstream log_msg;

	assert(redis_client_==NULL);
	redis_client_=new RedisClient();
	if(redis_client_==NULL)
	{
		LOG_WRITE_ERROR("create RedisClient failed");
		return RESTORER_FAIL;
	}
	//error_callback_=error_callback;
	redis_mode_=SENTINEL_MODE;

	int max_worker_thread_num=8;
	log_msg.str("");
	log_msg<<"max worker thead num is "<< max_worker_thread_num;
	LOG_WRITE_INFO(log_msg.str());

	data_restorer_background_threads_num_=1; 	
	log_msg.str("");
	log_msg<<"background_restorer_thread num is "<<data_restorer_background_threads_num_;
	LOG_WRITE_INFO(log_msg.str());
		
	int connection_num = max_worker_thread_num + data_restorer_background_threads_num_;
	log_msg.str("");
	log_msg<<"redis_connection num is "<<connection_num;
	LOG_WRITE_INFO(log_msg.str());
	
	std::string master_name="";
	uint32_t connect_timeout_ms=1500;
	uint32_t read_timeout_ms=3000;
	std::string passwd="";

	REDIS_SERVER_LIST redis_server_list;
	for (std::list<UrlPtr>::iterator it = restorer_param.urlList.begin(); it != restorer_param.urlList.end(); it++)
	{
		RedisServerInfo server((*it)->ip, (*it)->port);
		redis_server_list.push_back(server);
	}
	if(redis_client_->init(redis_mode_, redis_server_list, master_name, connection_num, connect_timeout_ms, read_timeout_ms, passwd)==false)
	{
		LOG_WRITE_ERROR("init RedisClient failed");
		delete redis_client_;
		redis_client_=NULL;
		return RESTORER_FAIL;
	}
	
	inited_=true;
	return RESTORER_OK;
}

int CDownDataRestorerRedis::Uninit()
{
	if(inited_  &&  redis_client_)
	{
		redis_client_->freeRedisClient();
		delete redis_client_;
		redis_client_=NULL;
	}
	
	LOG_WRITE_INFO("CDownDataRestorerRedis Uninit now");
	inited_=false;
	return RESTORER_OK;
}

int CDownDataRestorerRedis::Start()
{
	if(!inited_)
	{
		LOG_WRITE_WARNING("please init CDownDataRestorerRedis first");
		return RESTORER_FAIL;
	}

	if(started_)
	{
		LOG_WRITE_WARNING("start called repeatedly");
		return RESTORER_FAIL;
	}

	LOG_WRITE_INFO("CDownDataRestorerRedis Start now");

	device_mgr_=new DeviceMgr(redis_client_);
	channel_mgr_=new ChannelMgr(redis_client_);
	executing_invite_cmd_mgr_=new ExecutingInviteCmdMgr(redis_client_);
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
		return RESTORER_FAIL;
	}
	
	data_restorer_threads_.reserve(data_restorer_background_threads_num_);
	int ret=0;
	pthread_t thread_id;
	for(int i=0; i<data_restorer_background_threads_num_; i++)
	{		
		ret=pthread_create(&thread_id, NULL, DataRestorerThreadFuncWrapper, this);
		if(ret!=0)
		{
			LOG_WRITE_WARNING("pthread_create failed");
		}
		else
		{
			data_restorer_threads_.push_back(thread_id);
		}
	}
	if(data_restorer_threads_.empty())
	{
		LOG_WRITE_ERROR("start redis threads failed");
		return RESTORER_FAIL;
	}
	data_restorer_threads_.shrink_to_fit();
    
	data_restorer_threads_exit_latch=new CountDownLatch(data_restorer_threads_.size());
	if(data_restorer_threads_exit_latch==NULL)
	{
		LOG_WRITE_ERROR("new data_restorer_threads_exit_latch failed");
		return RESTORER_FAIL;
	}
	if(data_restorer_threads_exit_latch->IsValid()==false)
	{
		LOG_WRITE_ERROR("init CountDownLatch failed");
		delete data_restorer_threads_exit_latch;
		data_restorer_threads_exit_latch=NULL;

		KillDataRestorerThread();
		
		return RESTORER_FAIL;
	}
	
	started_=true;
	return RESTORER_OK;
}

int CDownDataRestorerRedis::Stop()
{
	CHECK_CONDITION(started_, "CDownDataRestorerRedis not started");
	LOG_WRITE_INFO("CDownDataRestorerRedis stop now");

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
	return RESTORER_OK;
}

void CDownDataRestorerRedis::KillDataRestorerThread()
{
	for(size_t i=0; i<data_restorer_threads_.size(); i++)
	{
		pthread_cancel(data_restorer_threads_[i]);
	}
}

void* CDownDataRestorerRedis::DataRestorerThreadFuncWrapper(void* arg)
{
	CDownDataRestorerRedis* down_data_restorer=(CDownDataRestorerRedis*)arg;
	down_data_restorer->DataRestorerThreadFunc();
	return 0;
}

void CDownDataRestorerRedis::DataRestorerThreadFunc()
{
	LOG_WRITE_INFO("restorer thread started");
	
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
		std::stringstream log_msg;
		log_msg<<"to process data_restorer_operation "<<operation.operation_type;
		LOG_WRITE_INFO(log_msg.str());


		if(operation.operation_type==DataRestorerOperation::INSERT)
		{
			if(operation.entity_type==DataRestorerOperation::DEVICE)
			{
				device_mgr_->InsertDevice(operation.gbdownlinker_device_id, *(Device*)(*operation.entities.begin()));
			}
			else if(operation.entity_type==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->InsertChannel(operation.gbdownlinker_device_id, *(Channel*)(*(operation.entities.begin())));
			}
			else if(operation.entity_type==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Insert(operation.gbdownlinker_device_id, operation.worker_thread_idx, *(ExecutingInviteCmd*)(*operation.entities.begin()));
			}
		}
		else if (operation.operation_type == DataRestorerOperation::UPDATE)
		{
			if (operation.entity_type == DataRestorerOperation::DEVICE)
			{
				device_mgr_->UpdateDevice(operation.gbdownlinker_device_id, *(Device*)(*operation.entities.begin()));
			}
			else if (operation.entity_type == DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->UpdateChannel(operation.gbdownlinker_device_id, *(Channel*)(*(operation.entities.begin())));
			}
			else if (operation.entity_type == DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Update(operation.gbdownlinker_device_id, operation.worker_thread_idx, *(ExecutingInviteCmd*)(*operation.entities.begin()));
			}
		}
		else if(operation.operation_type==DataRestorerOperation::DELETE)
		{
			if(operation.entity_type==DataRestorerOperation::DEVICE)
			{
				device_mgr_->DeleteDevice(operation.gbdownlinker_device_id, operation.entity_id);
			}
			else if(operation.entity_type==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->DeleteChannel(operation.gbdownlinker_device_id, operation.entity_id);
			}
			else if(operation.entity_type==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Delete(operation.gbdownlinker_device_id, operation.worker_thread_idx, operation.invite_cmd_sender_id, operation.invite_device_id, operation.invite_cmd_seq);
			}
		}
		else if(operation.operation_type==DataRestorerOperation::CLEAR)
		{
			if(operation.entity_type==DataRestorerOperation::DEVICE)
			{
				device_mgr_->ClearDevice(operation.gbdownlinker_device_id);
			}
			else if(operation.entity_type==DataRestorerOperation::CHANNEL)
			{
				channel_mgr_->ClearChannel(operation.gbdownlinker_device_id);
			}
			else if(operation.entity_type==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Clear(operation.gbdownlinker_device_id, operation.worker_thread_idx);
			}
		}
		
		for(list<void*>::iterator it=operation.entities.begin(); it!=operation.entities.end(); it++)
		{
			if(operation.entity_type==DataRestorerOperation::DEVICE)
			{
				delete (Device*)(*it);
			}
			else if(operation.entity_type==DataRestorerOperation::CHANNEL)
			{
				delete (Channel*)(*it);
			}
			else if(operation.entity_type==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				delete (ExecutingInviteCmd*)(*it);
			}
		}
	}

	data_restorer_threads_exit_latch->CountDown();
	LOG_WRITE_INFO("data restorer thread exits now");
}


int CDownDataRestorerRedis::GetStat(const std::string& gbdownlinker_device_id, int worker_thread_num)
{
	if (!started_)
	{
		LOG_WRITE_INFO("CDownDataRestorerRedis not started");
		return RESTORER_OK;
	}

	LOG_WRITE_INFO("CDownDataRestorerRedis stat: ");

	std::stringstream log_msg;
	log_msg <<"device count: "<<GetDeviceCount(gbdownlinker_device_id);
	LOG_WRITE_INFO(log_msg.str());

	log_msg.str("");
	log_msg <<"channel count: "<<GetChannelCount(gbdownlinker_device_id);
	LOG_WRITE_INFO(log_msg.str());

	for(int i=0; i<worker_thread_num; i++)
	{
		log_msg.str("");
		log_msg<<"ExecutingInviteCmdList["<<i<<"] count: "<<GetExecutingInviteCmdCount(gbdownlinker_device_id, worker_thread_num);
		LOG_WRITE_INFO(log_msg.str());
	}
	return RESTORER_OK;
}

int CDownDataRestorerRedis::GetDeviceCount(const std::string& gbdownlinker_device_id)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	return device_mgr_->GetDeviceCount(gbdownlinker_device_id);
}

int CDownDataRestorerRedis::GetChannelCount(const std::string& gbdownlinker_device_id)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	return channel_mgr_->GetChannelCount(gbdownlinker_device_id);
}

int CDownDataRestorerRedis::GetExecutingInviteCmdCount(const std::string& gbdownlinker_device_id, int worker_thread_no)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	return executing_invite_cmd_mgr_->GetExecutingInviteCmdListSize(gbdownlinker_device_id, worker_thread_no);
}

// compare channel.device_id to device.id
static bool CompareChannelDeviceId(const std::string& device_key, const std::string& channel_key_device_id)
{
	std::string device_id;
	std::string::size_type pos = device_key.rfind(':');
	if (pos == string::npos)
		return false;
	device_id = device_key.substr(pos+1);
	std::stringstream log_msg;
	log_msg << "device_key "<< device_key<<" has device_id " << device_id;
	LOG_WRITE_INFO(log_msg.str());
	return device_id == channel_key_device_id;
}

// compare invite.device_id to channel.channel_device_id
//static bool CompareInviteDeviceId(const std::string& channel_device_id, const std::string& invite_device_id)
//{
//	return invite_device_id == channel_device_id;
//}

int CDownDataRestorerRedis::PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::stringstream log_msg;
	
	gbdownlinker_device_id_ = gbdownlinker_device_id;

	// {downlinker.device}:gbdownlinker_device_id:device_id
	device_mgr_->GetDeviceKeyList(gbdownlinker_device_id, device_key_list_); // full device key

	// {downlinker.channel}:gbdownlinker_device_id:device_id:channel_device_id
	channel_mgr_->GetChannelKeyList(gbdownlinker_device_id, channel_id_list_); // full channel key

	std::list<std::string> channel_device_id_list;
	for (std::list<std::string>::iterator it = channel_id_list_.begin(); it != channel_id_list_.end(); )
	{
		std::string device_id_part;
		std::string channel_device_id;
		ParseChannelKey(*it, device_id_part, channel_device_id);
		log_msg.str("");
		log_msg << "parse channel key " << *it << ", get device_id " << device_id_part << " and channel_device_id " << channel_device_id;
		LOG_WRITE_INFO(log_msg.str());

		if (std::find(device_key_list_.cbegin(), device_key_list_.cend(), std::bind2nd(std::ptr_fun(CompareChannelDeviceId), device_id_part)) == device_key_list_.end£¨£©)
		{
			channel_mgr_->DeleteChannel(*it);

			channel_id_list_.erase(it++);
		}
		else
		{
			channel_device_id_list.push_back(channel_device_id);
			it++;
		}
	}

	executing_invite_cmd_mgr_->GetInviteKeyList(gbdownlinker_device_id, invite_key_list_);

	invite_key_lists_.resize(worker_thread_number);
	for (std::list<std::string>::iterator it = invite_key_list_.begin(); it != invite_key_list_.end(); )
	{
		int worker_thread_idx;
		std::string device_id_part;
		ParseInviteKey(*it, worker_thread_idx, device_id_part);
		log_msg.str("");
		log_msg << "parse invite key " << *it << ", get worker_thread_idx " << worker_thread_idx << " and device_id " << device_id_part;
		LOG_WRITE_INFO(log_msg.str());

		if (worker_thread_idx => worker_thread_number)
		{
			log_msg.str("");
			log_msg << "invite key " << *it << ", worker_thread_idx "<<worker_thread_idx<<" is invalid "<<worker_thread_number;
			LOG_WRITE_ERROR(log_msg.str());

			executing_invite_cmd_mgr_->DeleteInvite(*it);

			invite_key_list_.erase(it++);
			continue;
		}

		if (std::find(channel_device_id_list.begin(), channel_device_id_list.end(), device_id_part) == channel_device_id_list.end())
		{
			log_msg.str("");
			log_msg << "invite key " << *it << ", device_id " << device_id_part << " not exists in channel list";
			LOG_WRITE_ERROR(log_msg.str());

			executing_invite_cmd_mgr_->DeleteInvite(*it);

			invite_key_list_.erase(it++);
			continue;
		}

		log_msg.str("");
		log_msg << "invite key " << *it << "is valid";
		LOG_WRITE_INFO(log.str());

		invite_key_lists_[worker_thread_idx].insert(*it);
		it++;
	}

	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* device_list)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	if (gbdownlinker_device_id_ == gbdownlinker_device_id  &&  !device_key_list_.empty())
	{
		device_mgr_->LoadDevice(device_key_list_, device_list);
	}
	else
	{
		device_mgr_->LoadDevice(gbdownlinker_device_id, device_list);
	}
	return RESTORER_OK;
}

int CDownDataRestorerRedis::InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);
	
	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::INSERT;
	operation.entity_type = DataRestorerOperation::DEVICE;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	PushEntity(operation, device);

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device)
{
	return InsertDeviceList(gbdownlinker_device_id, device);
}

int CDownDataRestorerRedis::DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);

	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::DELETE;
	operation.entity_type = DataRestorerOperation::DEVICE;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	operation.entity_id_ = device_id;

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteDeviceList(const std::string& gbdownlinker_device_id)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);
	
	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::CLEAR;
	operation.entity_type = DataRestorerOperation::DEVICE;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists);
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	if (gbdownlinker_device_id_ == gbdownlinker_device_id  &&  !invite_key_lists_[worker_thread_idx].empty())
	{
		executing_invite_cmd_mgr_->LoadInvite(invite_key_lists_[worker_thread_idx], executing_invite_cmd_lists);
	}
	else
	{
		executing_invite_cmd_mgr_->LoadInvite(gbdownlinker_device_id, worker_thread_idx, executing_invite_cmd_lists);
	}
	return RESTORER_OK;
}


int CDownDataRestorerRedis::InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executing_invite_cmd)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);

	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::INSERT;
	operation.entity_type = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	operation.worker_thread_idx= worker_thread_idx;
	PushEntity(operation, executing_invite_cmd);

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.Signal();
	return RESTORER_OK;
}


int CDownDataRestorerRedis::UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executing_invite_cmd)
{
	return InsertExecutingInviteCmdList(gbdownlinker_device_id, worker_thread_idx, executing_invite_cmd);
}

int CDownDataRestorerRedis::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);
	
	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::DELETE;
	operation.entity_type = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	operation.worker_thread_idx = worker_thread_idx;
	operation.cmd_sender_id = cmd_sender_id;
	operation.device_id = device_id;
	operation.cmd_seq = cmd_seq;

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}


int CDownDataRestorerRedis::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);
	
	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::CLEAR;
	operation.entity_type = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	operation.worker_thread_idx = worker_thread_idx;

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}


} // namespace GBGateway


