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
#include<functional>

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

static bool ParseChannelKey(const std::string& channel_key, std::string& device_id, std::string& channel_device_id)
{
	std::string::size_type pos=channel_key.rfind(':');
	if(pos==std::string::npos)
		return false;
	channel_device_id=channel_key.substr(pos+1);

	std::string::size_type pos2=channel_key.rfind(':', pos-1);
	if(pos2==std::string::npos)
		return false;
	device_id=channel_key.substr(pos2+1, pos-(pos2+1));

	return true;
}

static bool ParseInviteKey(const std::string& invite_key, int& worker_thread_idx, std::string& device_id)
{
	std::string::size_type pos1=invite_key.rfind(':');	
	if(pos1==std::string::npos)
		return false;
	std::string::size_type pos2=invite_key.rfind(':', pos1-1);
	if(pos2==std::string::npos)
		return false;
	device_id=invite_key.substr(pos2+1, pos1-(pos2+1));	

	std::string::size_type pos3=invite_key.rfind(':', pos2-1);
	if(pos3==std::string::npos)
		return false;
	std::string::size_type pos4=invite_key.rfind(':', pos3-1);
	if(pos4==std::string::npos)
		return false;

	std::string worker_thread_idx_str=invite_key.substr(pos4+1, pos3-(pos4+1));
	worker_thread_idx=atoi(worker_thread_idx_str.c_str());
	return true;
}

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

int CDownDataRestorerRedis::Init(const RestorerParamPtr& restorer_param/*, ErrorReportCallback error_callback*/)
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
	
	REDIS_SERVER_LIST redis_server_list;
	const RestorerParam::RedisConfig& redis_config=std::get<RestorerParam::RedisConfig>(restorer_param->config);
	for (std::list<RestorerParam::RedisConfig::UrlPtr>::const_iterator it = redis_config.urlList.cbegin(); it != redis_config.urlList.cend(); it++)
	{
		RedisServerInfo server((*it)->ip, (*it)->port);
		redis_server_list.push_back(server);
	}
	std::string master_name=redis_config.masterName;

	uint32_t connect_timeout_ms=1500;
	uint32_t read_timeout_ms=3000;
	std::string passwd="";

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
				assert(operation.entities.size()==1);
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
				device_mgr_->DeleteDevice(operation.gbdownlinker_device_id, operation.device_id);
			}
			else if(operation.entity_type==DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				executing_invite_cmd_mgr_->Delete(operation.gbdownlinker_device_id, operation.worker_thread_idx, operation.invite_cmd_sender_id, operation.invite_device_id, operation.invite_cmd_seq);
			}
		}
		else if(operation.operation_type==DataRestorerOperation::DELETE_CHANNEL_BY_ID)
		{
			assert(operation.entity_type==DataRestorerOperation::CHANNEL);
			channel_mgr_->DeleteChannel(operation.gbdownlinker_device_id, operation.channel_device_id, operation.channel_channel_device_id);
		}
		else if(operation.operation_type==DataRestorerOperation::DELETE_CHANNEL_BY_DEVICE_ID)
		{
			assert(operation.entity_type==DataRestorerOperation::CHANNEL);
			channel_mgr_->DeleteChannel(operation.gbdownlinker_device_id, operation.channel_device_id);
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

int CDownDataRestorerRedis::GetExecutingInviteCmdCount(const std::string& gbdownlinker_device_id, int worker_thread_idx)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	return executing_invite_cmd_mgr_->GetInviteCount(gbdownlinker_device_id, worker_thread_idx);
}

// compare channel.device_id to device.id
static bool CompareChannelDeviceId(std::string device_key, const std::string& channel_key_device_id)
{
	std::string device_id;
	std::string::size_type pos = device_key.rfind(':');
	if (pos == string::npos)
		return false;
	device_id = device_key.substr(pos+1);
	return device_id == channel_key_device_id;
}

class ChannelDeviceIdComparator : public std::binary_function<std::string, std::string, bool>
{
public:
	bool operator()(const std::string& device_key, const std::string& channel_key_device_id) const 
	{
		std::string device_id;
		std::string::size_type pos = device_key.rfind(':');
		if (pos == string::npos)
			return false;
		device_id = device_key.substr(pos+1);
		return device_id == channel_key_device_id;
	}
	
};

void CDownDataRestorerRedis::ResetKeyList()
{
	gbdownlinker_device_id_.clear();
	device_key_list_.clear();
	channel_key_list_.clear();
	invite_key_list_.clear();
	invite_key_lists_.clear();
}

void CDownDataRestorerRedis::DebugPrint(const std::list<std::string>& key_list)
{
	for(std::list<std::string>::const_iterator it=key_list.begin(); it!=key_list.end(); ++it)
	{
		LOG_WRITE_INFO(*it);
	}
}

int CDownDataRestorerRedis::PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");

	ResetKeyList();

	std::stringstream log_msg;
	
	gbdownlinker_device_id_ = gbdownlinker_device_id;

	// {downlinker.device}:gbdownlinker_device_id:device_id
	device_mgr_->GetDeviceKeyList(gbdownlinker_device_id, device_key_list_); // full device key
//	DebugPrint(device_key_list_);

	// {downlinker.channel}:gbdownlinker_device_id:device_id:channel_device_id
	channel_mgr_->GetChannelKeyList(gbdownlinker_device_id, channel_key_list_); // full channel key
//	DebugPrint(channel_key_list_);

	std::list<std::string> channel_device_id_list;
	for (std::list<std::string>::iterator it = channel_key_list_.begin(); it != channel_key_list_.end(); )
	{
		std::string device_id_part;
		std::string channel_device_id;
		ParseChannelKey(*it, device_id_part, channel_device_id);
//		log_msg.str("");
//		log_msg << "parse channel key " << *it << ", get device_id " << device_id_part << " and channel_device_id " << channel_device_id;
//		LOG_WRITE_INFO(log_msg.str());

		if (std::find_if(device_key_list_.cbegin(), device_key_list_.cend(), std::bind2nd(std::ptr_fun(CompareChannelDeviceId), device_id_part)) == device_key_list_.end())
		//if (std::find_if(device_key_list_.cbegin(), device_key_list_.cend(), std::bind2nd(ChannelDeviceIdComparator(), device_id_part)) == device_key_list_.end())
		{
			log_msg.str("");
			log_msg<<"channel has wrong device_id "<<device_id_part;
			LOG_WRITE_INFO(log_msg.str());

			channel_mgr_->DeleteChannel(*it);

			channel_key_list_.erase(it++);
		}
		else
		{
//			log_msg.str("");
//			log_msg<<"channel is valid, channel_device_id is "<<channel_device_id;
//			LOG_WRITE_INFO(log_msg.str());

			channel_device_id_list.push_back(channel_device_id);
			it++;
		}
	}

	executing_invite_cmd_mgr_->GetInviteKeyList(gbdownlinker_device_id, invite_key_list_);
	DebugPrint(invite_key_list_);

	invite_key_lists_.resize(worker_thread_number);
	for (std::list<std::string>::iterator it = invite_key_list_.begin(); it != invite_key_list_.end(); )
	{
		int worker_thread_idx;
		std::string device_id_part;
		ParseInviteKey(*it, worker_thread_idx, device_id_part);
//		log_msg.str("");
//		log_msg << "parse invite key " << *it << ", get worker_thread_idx " << worker_thread_idx << " and device_id " << device_id_part;
//		LOG_WRITE_INFO(log_msg.str());

		if (worker_thread_idx >= worker_thread_number)
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
			log_msg << "invite key " << *it << ", device_id " << device_id_part << " not exists in channel_device_id list";
			LOG_WRITE_ERROR(log_msg.str());

			executing_invite_cmd_mgr_->DeleteInvite(*it);

			invite_key_list_.erase(it++);
			continue;
		}

//		log_msg.str("");
//		log_msg << "invite key " << *it << " is valid";
//		LOG_WRITE_INFO(log_msg.str());

		invite_key_lists_[worker_thread_idx].push_back(*it);
		it++;
	}

	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* device_list)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	device_list->clear();
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
	CHECK_CONDITION(PushEntity(operation, device), "push entity failed");

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
	operation.device_id = device_id;

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

int CDownDataRestorerRedis::LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channel_list)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	channel_list->clear();
	if (gbdownlinker_device_id_ == gbdownlinker_device_id  &&  !channel_key_list_.empty())
	{
		channel_mgr_->LoadChannel(channel_key_list_, channel_list);
	}
	else
	{
		channel_mgr_->LoadChannel(gbdownlinker_device_id, channel_list);
	}
	return RESTORER_OK;
}

int CDownDataRestorerRedis::InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);
	
	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::INSERT;
	operation.entity_type = DataRestorerOperation::CHANNEL;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	CHECK_CONDITION(PushEntity(operation, channel), "push entity failed");

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel)
{
	return InsertChannelList(gbdownlinker_device_id, channel);
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);

	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::DELETE_CHANNEL_BY_DEVICE_ID;
	operation.entity_type = DataRestorerOperation::CHANNEL;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	operation.channel_device_id= device_id;

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& channel_device_id)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);

	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::DELETE_CHANNEL_BY_ID;
	operation.entity_type = DataRestorerOperation::CHANNEL;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	operation.channel_device_id= device_id;
	operation.channel_channel_device_id=channel_device_id;

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbdownlinker_device_id)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);
	
	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::CLEAR;
	operation.entity_type = DataRestorerOperation::CHANNEL;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.SignalAll();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	executing_invite_cmd_lists->clear();
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
	CHECK_CONDITION(PushEntity(operation, executing_invite_cmd), "push entity failed");

	operations_queue_.push(operation);
	operations_queue_not_empty_condvar.Signal();
	return RESTORER_OK;
}


int CDownDataRestorerRedis::UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executing_invite_cmd)
{
	return InsertExecutingInviteCmdList(gbdownlinker_device_id, worker_thread_idx, executing_invite_cmd);
}

int CDownDataRestorerRedis::DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& invite_cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	MutexLockGuard guard(&operations_queue_mutex_);
	
	DataRestorerOperation operation;
	operation.operation_type = DataRestorerOperation::DELETE;
	operation.entity_type = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbdownlinker_device_id = gbdownlinker_device_id;
	operation.worker_thread_idx = worker_thread_idx;
	operation.invite_cmd_sender_id = invite_cmd_sender_id;
	operation.invite_device_id = device_id;
	operation.invite_cmd_seq = cmd_seq;

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


