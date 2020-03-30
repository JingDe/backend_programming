#include "down_data_restorer_redis.h"
#include "device.h"
#include "channel.h"
#include "executing_invite_cmd.h"
#include "device_mgr.h"
#include "channel_mgr.h"
#include "executing_invite_cmd_mgr.h"
#include "base_library/log.h"

#include <sstream>
#include <cassert>
#include <functional>
#include <exception>

namespace GBDownLinker {


static bool ParseChannelKey(const std::string& channel_key, std::string& device_id, std::string& channelDeviceId)
{
	std::string::size_type pos = channel_key.rfind(':');
	if (pos == std::string::npos)
		return false;
	channelDeviceId = channel_key.substr(pos + 1);

	std::string::size_type pos2 = channel_key.rfind(':', pos - 1);
	if (pos2 == std::string::npos)
		return false;
	device_id = channel_key.substr(pos2 + 1, pos - (pos2 + 1));

	return true;
}

static bool ParseInviteKey(const std::string& invite_key, int& workerThreadIdx, std::string& device_id)
{
	std::string::size_type pos1 = invite_key.rfind(':');
	if (pos1 == std::string::npos)
		return false;
	std::string::size_type pos2 = invite_key.rfind(':', pos1 - 1);
	if (pos2 == std::string::npos)
		return false;
	device_id = invite_key.substr(pos2 + 1, pos1 - (pos2 + 1));

	std::string::size_type pos3 = invite_key.rfind(':', pos2 - 1);
	if (pos3 == std::string::npos)
		return false;
	std::string::size_type pos4 = invite_key.rfind(':', pos3 - 1);
	if (pos4 == std::string::npos)
		return false;

	std::string workerThreadIdx_str = invite_key.substr(pos4 + 1, pos3 - (pos4 + 1));
	workerThreadIdx = atoi(workerThreadIdx_str.c_str());
	return true;
}

// compare channel.device_id to device.id
static bool CompareChannelDeviceId(std::string device_key, const std::string& channel_key_device_id)
{
	std::string device_id;
	std::string::size_type pos = device_key.rfind(':');
	if (pos == string::npos)
		return false;
	device_id = device_key.substr(pos + 1);
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
		device_id = device_key.substr(pos + 1);
		return device_id == channel_key_device_id;
	}
};


CDownDataRestorerRedis::CDownDataRestorerRedis()
	:redisMode_(STAND_ALONE_OR_PROXY_MODE),
	redisClient_(NULL),
	inited_(false),
	started_(false),
	forceThreadExit_(false),
	dataRestorerThread_(),
	deviceMgr_(NULL),
	channelMgr_(NULL),
	inviteMgr_(NULL),
	operationQueueMutex_(),
	operationQueueNotEmptyCondvar_(),
	operationQueue_(),
//	maxQueueSize_(30000),
	unrecoverableQueueThreshold(30000),
	workStatus_(Normal),
	needClearBackupData_(false)
{
	LOG_WRITE_INFO("CDownDataRestorerRedis constructed ok");
}

CDownDataRestorerRedis::~CDownDataRestorerRedis()
{
	LOG_WRITE_INFO("CDownDataRestorerRedis descontruction");

	if (started_)
		Stop();
	if (inited_)
		Uninit();
}

void CDownDataRestorerRedis::RedisClientCallback(RedisClientStatus clientStatus)
{
	std::stringstream log_msg;
	log_msg << "RedisClient status transfer to " << clientStatus;
	LOG_WRITE_INFO(log_msg.str());

	if(workStatus_==UnrecoverableException)
	{
		// ignore
		StopRedisClient();
		return;
	}

	if (workStatus_ == Normal  &&  clientStatus != RedisClientNormal)
	{
		workStatus_ = RecoverableException;

		//CManager::AddException(Exception::RESTORER);
		callback_(Unhealthy);
		return;
	}
	
	if (workStatus_ == RecoverableException  &&  clientStatus == RedisClientNormal)
	{
		workStatus_ = Normal;
		if (needClearBackupData_)
		{
			ClearBackupData();
			needClearBackupData_ = false;
		}
	
		callback_(Healthy);
	}
}

int CDownDataRestorerRedis::Init(const RestorerParamPtr& restorer_param, RestorerWorkHealthy initialRestorerWorkHealthy, std::function<void(RestorerWorkHealthy)> callback)
{
	if (inited_)
	{
		LOG_WRITE_WARNING("init called repeatedly");
		return RESTORER_OK;
	}

	assert(restorer_param->type == RestorerParam::Type::Redis);

	std::stringstream log_msg;

	needClearBackupData_ = false;

	assert(redisClient_ == NULL);
	redisClient_ = new RedisClient();
	if (redisClient_ == NULL)
	{
		LOG_WRITE_FATAL("create RedisClient failed");
		workStatus_=UnrecoverableException;
		StopRedisClient();
		if(initialRestorerWorkHealthy==true)
		{
			callback_(Unhealthy);
		}
		return RESTORER_FAIL;
	}
	callback_=callback;
	redisMode_ = SENTINEL_MODE;

	REDIS_SERVER_LIST redis_server_list;
	const RestorerParam::RedisConfig& redis_config = std::get<RestorerParam::RedisConfig>(restorer_param->config);
	for (std::list<RestorerParam::RedisConfig::UrlPtr>::const_iterator it = redis_config.urlList.cbegin(); it != redis_config.urlList.cend(); it++)
	{
		RedisServerInfo server((*it)->ip, (*it)->port);
		redis_server_list.push_back(server);
	}
	std::string master_name = redis_config.masterName;

	uint32_t connect_timeout_ms = 1500;
	uint32_t read_timeout_ms = 3000;
	std::string passwd = "";

	RedisClientInitResult init_result = redisClient_->init(redisMode_, redis_server_list, master_name, 1, connect_timeout_ms, read_timeout_ms, passwd);

	if (initialRestorerWorkHealthy == true)
	{
		if (init_result == InitSuccess)
		{
			workStatus_ = Normal;
			inited_=true;
			
			return RESTORER_OK;
		}
		else if(init_result==RecoverableFail)
		{
			workStatus_ = RecoverableException;
			needClearBackupData_ = true;
			inited_=true;
			
			callback_(Unhealthy);
			return RESTORER_OK;
		}
		else if(init_result==UnrecoverableFail)
		{
			workStatus_=UnrecoverableException;
			StopRedisClient();
			inited_=false;
			
			callback_(Unhealthy);
			return RESTORER_FAIL;
		}
	}
	else
	{
		if (init_result == InitSuccess)
		{
			workStatus_ = Normal;
			needClearBackupData_ = true;
			inited_=true;
			
			callback_(Healthy);
			return RESTORER_OK;
		}
		else if(init_result==RecoverableFail)
		{
			workStatus_ = RecoverableException;
			needClearBackupData_ = true;
			inited_=true;
			
			return RESTORER_OK;
		}
		else
		{
			workStatus_=UnrecoverableException;
			StopRedisClient();
			inited_=false;
			
			return RESTORER_FAIL;
		}
	}
}


int CDownDataRestorerRedis::Uninit()
{
	if (inited_ && redisClient_)
	{
		redisClient_->freeRedisClient();
		delete redisClient_;
		redisClient_ = NULL;
	}

	LOG_WRITE_INFO("CDownDataRestorerRedis Uninit now");
	inited_ = false;
	return RESTORER_OK;
}

void CDownDataRestorerRedis::StopRedisClient()
{
	if(redisClient_)
	{
		redisClient_->freeRedisClient();
		delete redisClient_;
		redisClient_=NULL;
	}

	// TODO
//	Stop();
}

int CDownDataRestorerRedis::Start()
{
	if (!inited_)
	{
		LOG_WRITE_ERROR("please init CDownDataRestorerRedis first");
		return RESTORER_FAIL;
	}

	if (started_)
	{
		LOG_WRITE_WARNING("CDownDataRestorerRedis has started");
		return RESTORER_OK;
	}

	if(workStatus_==UnrecoverableException)
	{
		StopRedisClient();
	
		callback_(Unhealthy);
		return RESTORER_FAIL;
	}

	LOG_WRITE_INFO("CDownDataRestorerRedis Start now");

	deviceMgr_ = new DeviceMgr(redisClient_);
	channelMgr_ = new ChannelMgr(redisClient_);
	inviteMgr_ = new ExecutingInviteCmdMgr(redisClient_);
	if (!deviceMgr_ || !channelMgr_ || !inviteMgr_)
	{
		goto CLEAR;
	}

	try
	{
		dataRestorerThread_ = std::make_unique<std::thread>(&CDownDataRestorerRedis::DataRestorerThreadFuncWrapper, this);
	}
	catch (const std::exception & e)
	{
		LOG_WRITE_FATAL("start data restorer thread failed");		
		goto CLEAR;
	}

	started_ = true;
	return RESTORER_OK;

CLEAR:
	StopRestorerMgr();
	workStatus_=UnrecoverableException;
	StopRedisClient();
	
	callback_(Unhealthy);
	return RESTORER_FAIL;
}

int CDownDataRestorerRedis::Stop()
{
	CHECK_CONDITION_LOG(started_, "CDownDataRestorerRedis not started");
	LOG_WRITE_INFO("CDownDataRestorerRedis stop now");

	StopDataRestorerThread();

	StopRestorerMgr();

	started_ = false;
	return RESTORER_OK;
}

void CDownDataRestorerRedis::StopRestorerMgr()
{
	if (deviceMgr_)
	{
		delete deviceMgr_;
		deviceMgr_ = NULL;
	}
	if (channelMgr_)
	{
		delete channelMgr_;
		channelMgr_ = NULL;
	}
	if (inviteMgr_)
	{
		delete inviteMgr_;
		inviteMgr_ = NULL;
	}
}

void CDownDataRestorerRedis::StopDataRestorerThread()
{
	forceThreadExit_ = true;
	operationQueueNotEmptyCondvar_.notify_all();
	dataRestorerThread_->join();
}

void CDownDataRestorerRedis::DataRestorerThreadFuncWrapper(void* arg)
{
	CDownDataRestorerRedis* down_data_restorer = (CDownDataRestorerRedis*)arg;
	down_data_restorer->DataRestorerThreadFunc();
}

void CDownDataRestorerRedis::ClearOperationQueue()
{
	std::lock_guard<std::mutex> guard(operationQueueMutex_);
	operationQueue_.clear();
}

void CDownDataRestorerRedis::DataRestorerThreadFunc()
{
	LOG_WRITE_INFO("restorer thread started");

	while (!forceThreadExit_)
	{
		if (workStatus_ == UnrecoverableException)
		{
			ClearOperationQueue();
			StopRedisClient();
			callback_(Unhealthy);
			break;
		}
		else if (workStatus_ == RecoverableException)
		{
			size_t queue_size = 0;
			{
				std::lock_guard<std::mutex> guard(operationQueueMutex_);
				queue_size = operationQueue_.size();
			}

			if (queue_size > unrecoverableQueueThreshold)
			{
				workStatus_ = UnrecoverableException;
				ClearOperationQueue();
				StopRedisClient();
				callback_(Unhealthy);
				break;
			}
			else
			{
				continue;
			}
		}

		assert(workStatus_ == Normal);

		DataRestorerOperation operation;
		{
			std::unique_lock<std::mutex> lock(operationQueueMutex_);
			while (operationQueue_.empty())
			{
				if (forceThreadExit_)
					break;
				operationQueueNotEmptyCondvar_.wait_for(lock, std::chrono::seconds(5));
			}
			if (forceThreadExit_)
				break;
			assert(!operationQueue_.empty());
			operation = operationQueue_.front();
			operationQueue_.pop();
		}
		std::stringstream log_msg;
		log_msg << "to process data_restorer_operation " << operation.operationType;
		LOG_WRITE_INFO(log_msg.str());

		if (operation.operationType == DataRestorerOperation::INSERT)
		{
			if (operation.entityType == DataRestorerOperation::DEVICE)
			{
				assert(operation.entities.size() == 1);
				deviceMgr_->InsertDevice(operation.gbDownlinkerDeviceId, *(Device*)(*operation.entities.begin()));
			}
			else if (operation.entityType == DataRestorerOperation::CHANNEL)
			{
				channelMgr_->InsertChannel(operation.gbDownlinkerDeviceId, *(Channel*)(*(operation.entities.begin())));
			}
			else if (operation.entityType == DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				inviteMgr_->InsertInvite(operation.gbDownlinkerDeviceId, operation.workerThreadIdx, *(ExecutingInviteCmd*)(*operation.entities.begin()));
			}
		}
		else if (operation.operationType == DataRestorerOperation::UPDATE)
		{
			if (operation.entityType == DataRestorerOperation::DEVICE)
			{
				deviceMgr_->UpdateDevice(operation.gbDownlinkerDeviceId, *(Device*)(*operation.entities.begin()));
			}
			else if (operation.entityType == DataRestorerOperation::CHANNEL)
			{
				channelMgr_->UpdateChannel(operation.gbDownlinkerDeviceId, *(Channel*)(*(operation.entities.begin())));
			}
			else if (operation.entityType == DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				inviteMgr_->UpdateInvite(operation.gbDownlinkerDeviceId, operation.workerThreadIdx, *(ExecutingInviteCmd*)(*operation.entities.begin()));
			}
		}
		else if (operation.operationType == DataRestorerOperation::DELETE)
		{
			if (operation.entityType == DataRestorerOperation::DEVICE)
			{
				deviceMgr_->DeleteDevice(operation.gbDownlinkerDeviceId, operation.deviceId);
			}
			else if (operation.entityType == DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				inviteMgr_->DeleteInvite(operation.gbDownlinkerDeviceId, operation.workerThreadIdx, operation.inviteCmdSenderId, operation.inviteDeviceId, operation.inviteCmdSeq);
			}
		}
		else if (operation.operationType == DataRestorerOperation::DELETE_CHANNEL_BY_ID)
		{
			assert(operation.entityType == DataRestorerOperation::CHANNEL);
			channelMgr_->DeleteChannel(operation.gbDownlinkerDeviceId, operation.channelDeviceId, operation.channelChannelDeviceId);
		}
		else if (operation.operationType == DataRestorerOperation::DELETE_CHANNEL_BY_DEVICE_ID)
		{
			assert(operation.entityType == DataRestorerOperation::CHANNEL);
			channelMgr_->DeleteChannel(operation.gbDownlinkerDeviceId, operation.channelDeviceId);
		}
		else if (operation.operationType == DataRestorerOperation::CLEAR)
		{
			if (operation.entityType == DataRestorerOperation::DEVICE)
			{
				deviceMgr_->ClearDevice(operation.gbDownlinkerDeviceId);
			}
			else if (operation.entityType == DataRestorerOperation::CHANNEL)
			{
				channelMgr_->ClearChannel(operation.gbDownlinkerDeviceId);
			}
			else if (operation.entityType == DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				inviteMgr_->ClearInvite(operation.gbDownlinkerDeviceId, operation.workerThreadIdx);
			}
		}

		for (list<void*>::iterator it = operation.entities.begin(); it != operation.entities.end(); it++)
		{
			if (operation.entityType == DataRestorerOperation::DEVICE)
			{
				delete (Device*)(*it);
			}
			else if (operation.entityType == DataRestorerOperation::CHANNEL)
			{
				delete (Channel*)(*it);
			}
			else if (operation.entityType == DataRestorerOperation::EXECUTING_INVITE_CMD)
			{
				delete (ExecutingInviteCmd*)(*it);
			}
		}
	}

	LOG_WRITE_INFO("data restorer thread exits now");
}


int CDownDataRestorerRedis::GetStats()
{
	return RESTORER_OK;
}


int CDownDataRestorerRedis::ClearBackupData(const std::string& gbDownlinkerDeviceId, int worker_thread_number)
{	
	if (started_)
	{
		deviceMgr_->ClearDevice(gbDownlinkerDeviceId);
		channelMgr_->ClearChannel(gbDownlinkerDeviceId);

		for (int idx = 0; i < worker_thread_number; ++idx)
			inviteMgr_->ClearInvite(gbDownlinkerDeviceId, idx);

		return RESTORER_OK;
	}
	return RESTORER_FAIL;
}

int CDownDataRestorerRedis::PrepareLoadData(const std::string& gbDownlinkerDeviceId, int worker_thread_number)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	
	if (needClearBackupData_)
	{
		ClearBackupData();
		needClearBackupData_ = false;
		return RESTORER_OK;
	}
	
	if (workStatus_ != Normal)
	{
		return RESTORER_OK;
	}

	std::list<std::string> deviceKeyList;
	std::list<std::string> channelKeyList;
	std::list<std::string> inviteKeyList;

	std::stringstream log_msg;

	deviceMgr_->GetDeviceKeyList(gbDownlinkerDeviceId, deviceKeyList);

	channelMgr_->GetChannelKeyList(gbDownlinkerDeviceId, channelKeyList);

	std::list<std::string> channelDeviceId_list;
	for (std::list<std::string>::iterator it = channelKeyList.begin(); it != channelKeyList.end(); )
	{
		std::string device_id_part;
		std::string channelDeviceId;
		ParseChannelKey(*it, device_id_part, channelDeviceId);

		if (std::find_if(deviceKeyList.cbegin(), deviceKeyList.cend(), std::bind2nd(std::ptr_fun(CompareChannelDeviceId), device_id_part)) == deviceKeyList.end())
			//if (std::find_if(deviceKeyList.cbegin(), deviceKeyList.cend(), std::bind2nd(ChannelDeviceIdComparator(), device_id_part)) == deviceKeyList.end())
		{
			log_msg.str("");
			log_msg << "channel has wrong device_id " << device_id_part;
			LOG_WRITE_INFO(log_msg.str());

			channelMgr_->DeleteChannel(*it);

			channelKeyList.erase(it++);
		}
		else
		{
			channelDeviceId_list.push_back(channelDeviceId);
			it++;
		}
	}

	inviteMgr_->GetInviteKeyList(gbDownlinkerDeviceId, inviteKeyList);

	for (std::list<std::string>::iterator it = inviteKeyList.begin(); it != inviteKeyList.end(); )
	{
		int workerThreadIdx;
		std::string device_id_part;
		ParseInviteKey(*it, workerThreadIdx, device_id_part);

		if (workerThreadIdx >= worker_thread_number)
		{
			log_msg.str("");
			log_msg << "invite key " << *it << ", workerThreadIdx " << workerThreadIdx << " is invalid " << worker_thread_number;
			LOG_WRITE_ERROR(log_msg.str());

			inviteMgr_->DeleteInvite(*it);

			inviteKeyList.erase(it++);
			continue;
		}

		if (std::find(channelDeviceId_list.begin(), channelDeviceId_list.end(), device_id_part) == channelDeviceId_list.end())
		{
			log_msg.str("");
			log_msg << "invite key " << *it << ", device_id " << device_id_part << " not exists in channelDeviceId list";
			LOG_WRITE_ERROR(log_msg.str());

			inviteMgr_->DeleteInvite(*it);

			inviteKeyList.erase(it++);
			continue;
		}

		it++;
	}

	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadDeviceList(const std::string& gbDownlinkerDeviceId, std::list<DevicePtr>* device_list)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");

	if(workStatus_==Normal)
		deviceMgr_->LoadDevice(gbDownlinkerDeviceId, device_list);

	return RESTORER_OK;
}

int CDownDataRestorerRedis::InsertDeviceList(const std::string& gbDownlinkerDeviceId, Device* device)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);

	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::INSERT;
	operation.entityType = DataRestorerOperation::DEVICE;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	CHECK_CONDITION_LOG(PushEntity(operation, device), "push entity failed");

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::UpdateDeviceList(const std::string& gbDownlinkerDeviceId, Device* device)
{
	return InsertDeviceList(gbDownlinkerDeviceId, device);
}

int CDownDataRestorerRedis::DeleteDeviceList(const std::string& gbDownlinkerDeviceId, const std::string& device_id)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE;
	operation.entityType = DataRestorerOperation::DEVICE;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.deviceId = device_id;

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteDeviceList(const std::string& gbDownlinkerDeviceId)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);

	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::CLEAR;
	operation.entityType = DataRestorerOperation::DEVICE;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadChannelList(const std::string& gbDownlinkerDeviceId, std::list<ChannelPtr>* channel_list)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");

	if(workStatus_==Normal)
		channelMgr_->LoadChannel(gbDownlinkerDeviceId, channel_list);
	
	return RESTORER_OK;
}

int CDownDataRestorerRedis::InsertChannelList(const std::string& gbDownlinkerDeviceId, Channel* channel)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::INSERT;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	CHECK_CONDITION_LOG(PushEntity(operation, channel), "push entity failed");

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::UpdateChannelList(const std::string& gbDownlinkerDeviceId, Channel* channel)
{
	return InsertChannelList(gbDownlinkerDeviceId, channel);
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbDownlinkerDeviceId, const std::string& device_id)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE_CHANNEL_BY_DEVICE_ID;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.channelDeviceId = device_id;

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbDownlinkerDeviceId, const std::string& device_id, const std::string& channelDeviceId)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE_CHANNEL_BY_ID;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.channelDeviceId = device_id;
	operation.channelChannelDeviceId = channelDeviceId;

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbDownlinkerDeviceId)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::CLEAR;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");

	if(workStatus_==Normal)
	{
		inviteMgr_->LoadInvite(gbDownlinkerDeviceId, workerThreadIdx, executing_invite_cmd_lists);
	}
	return RESTORER_OK;
}


int CDownDataRestorerRedis::InsertExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx, ExecutingInviteCmd* executing_invite_cmd)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::INSERT;
	operation.entityType = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.workerThreadIdx = workerThreadIdx;
	CHECK_CONDITION_LOG(PushEntity(operation, executing_invite_cmd), "push entity failed");

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}


int CDownDataRestorerRedis::UpdateExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx, ExecutingInviteCmd* executing_invite_cmd)
{
	return InsertExecutingInviteCmdList(gbDownlinkerDeviceId, workerThreadIdx, executing_invite_cmd);
}

int CDownDataRestorerRedis::DeleteExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx, const std::string& inviteCmdSenderId, const std::string& device_id, const int64_t& cmd_seq)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE;
	operation.entityType = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.workerThreadIdx = workerThreadIdx;
	operation.inviteCmdSenderId = inviteCmdSenderId;
	operation.inviteDeviceId = device_id;
	operation.inviteCmdSeq = cmd_seq;

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}


int CDownDataRestorerRedis::DeleteExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx)
{
	CHECK_CONDITION_LOG(started_, "please start CDownDataRestorerRedis first");
	CHECK_CONDITION(workStatus_!=UnrecoverableException);
	
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::CLEAR;
	operation.entityType = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.workerThreadIdx = workerThreadIdx;

//	if (operationQueue_.size() > maxQueueSize_)
//		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}


} // namespace GBDownLinker


