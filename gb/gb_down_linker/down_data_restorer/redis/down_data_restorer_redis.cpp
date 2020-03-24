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

#define CHECK_CONDITION(condition, err_msg) do{\
			if(!condition)\
			{\
				std::stringstream log_msg;\
				log_msg << err_msg;\
				LOG_WRITE_ERROR(log_msg.str());\
				return RESTORER_FAIL;\
			}\
		}while(0);

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
	maxQueueSize_(1000)
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

int CDownDataRestorerRedis::Init(const RestorerParamPtr& restorer_param/*, ErrorReportCallback error_callback*/)
{
	if (inited_)
	{
		LOG_WRITE_WARNING("init called repeatedly");
		return RESTORER_FAIL;
	}

	assert(restorer_param->type == RestorerParam::Type::Redis);

	std::stringstream log_msg;

	assert(redisClient_ == NULL);
	redisClient_ = new RedisClient();
	if (redisClient_ == NULL)
	{
		LOG_WRITE_ERROR("create RedisClient failed");
		return RESTORER_FAIL;
	}
	//errorCallback_=error_callback;
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

	if (redisClient_->init(redisMode_, redis_server_list, master_name, 1, connect_timeout_ms, read_timeout_ms, passwd) == false)
	{
		LOG_WRITE_ERROR("init RedisClient failed");
		delete redisClient_;
		redisClient_ = NULL;
		return RESTORER_FAIL;
	}

	inited_ = true;
	return RESTORER_OK;
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

int CDownDataRestorerRedis::Start()
{
	if (!inited_)
	{
		LOG_WRITE_WARNING("please init CDownDataRestorerRedis first");
		return RESTORER_FAIL;
	}

	if (started_)
	{
		LOG_WRITE_WARNING("start called repeatedly");
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
		dataRestorerThread_=std::make_unique<std::thread>(&CDownDataRestorerRedis::DataRestorerThreadFuncWrapper, this);
	} 
	catch (const std::exception& e)
	{
		LOG_WRITE_ERROR("start data restorer thread failed");
		goto CLEAR;
	}

	started_ = true;
	return RESTORER_OK;

CLEAR:
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
	return RESTORER_FAIL;
}

int CDownDataRestorerRedis::Stop()
{
	CHECK_CONDITION(started_, "CDownDataRestorerRedis not started");
	LOG_WRITE_INFO("CDownDataRestorerRedis stop now");

	StopDataRestorerThread();	
	
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

	started_ = false;
	return RESTORER_OK;
}

void CDownDataRestorerRedis::StopDataRestorerThread()
{
	forceThreadExit_=true;
	operationQueueNotEmptyCondvar_.notify_all();
	dataRestorerThread_->join();
}

void CDownDataRestorerRedis::DataRestorerThreadFuncWrapper(void* arg)
{
	CDownDataRestorerRedis* down_data_restorer = (CDownDataRestorerRedis*)arg;
	down_data_restorer->DataRestorerThreadFunc();
}

void CDownDataRestorerRedis::DataRestorerThreadFunc()
{
	LOG_WRITE_INFO("restorer thread started");

	while (!forceThreadExit_)
	{
		DataRestorerOperation operation;
		{
			std::unique_lock<std::mutex> lock(operationQueueMutex_);
			while (operationQueue_.empty())
			{
				if (forceThreadExit_)
					break;
				operationQueueNotEmptyCondvar_.wait(lock);
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

void CDownDataRestorerRedis::DebugPrint(const std::list<std::string>& key_list)
{
	for (std::list<std::string>::const_iterator it = key_list.begin(); it != key_list.end(); ++it)
	{
		LOG_WRITE_INFO(*it);
	}
}

int CDownDataRestorerRedis::PrepareLoadData(const std::string& gbDownlinkerDeviceId, int worker_thread_number)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");

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
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	device_list->clear();
	{
		deviceMgr_->LoadDevice(gbDownlinkerDeviceId, device_list);
	}
	return RESTORER_OK;
}

int CDownDataRestorerRedis::InsertDeviceList(const std::string& gbDownlinkerDeviceId, Device* device)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::INSERT;
	operation.entityType = DataRestorerOperation::DEVICE;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	CHECK_CONDITION(PushEntity(operation, device), "push entity failed");

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();

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
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE;
	operation.entityType = DataRestorerOperation::DEVICE;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.deviceId = device_id;

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteDeviceList(const std::string& gbDownlinkerDeviceId)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::CLEAR;
	operation.entityType = DataRestorerOperation::DEVICE;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadChannelList(const std::string& gbDownlinkerDeviceId, std::list<ChannelPtr>* channel_list)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	channel_list->clear();
	{
		channelMgr_->LoadChannel(gbDownlinkerDeviceId, channel_list);
	}
	return RESTORER_OK;
}

int CDownDataRestorerRedis::InsertChannelList(const std::string& gbDownlinkerDeviceId, Channel* channel)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::INSERT;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	CHECK_CONDITION(PushEntity(operation, channel), "push entity failed");

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
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
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE_CHANNEL_BY_DEVICE_ID;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.channelDeviceId = device_id;

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbDownlinkerDeviceId, const std::string& device_id, const std::string& channelDeviceId)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE_CHANNEL_BY_ID;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.channelDeviceId = device_id;
	operation.channelChannelDeviceId = channelDeviceId;

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::DeleteChannelList(const std::string& gbDownlinkerDeviceId)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::CLEAR;
	operation.entityType = DataRestorerOperation::CHANNEL;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}

int CDownDataRestorerRedis::LoadExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	executing_invite_cmd_lists->clear();
	{
		inviteMgr_->LoadInvite(gbDownlinkerDeviceId, workerThreadIdx, executing_invite_cmd_lists);
	}
	return RESTORER_OK;
}


int CDownDataRestorerRedis::InsertExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx, ExecutingInviteCmd* executing_invite_cmd)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::INSERT;
	operation.entityType = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.workerThreadIdx = workerThreadIdx;
	CHECK_CONDITION(PushEntity(operation, executing_invite_cmd), "push entity failed");

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
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
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::DELETE;
	operation.entityType = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.workerThreadIdx = workerThreadIdx;
	operation.inviteCmdSenderId = inviteCmdSenderId;
	operation.inviteDeviceId = device_id;
	operation.inviteCmdSeq = cmd_seq;

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}


int CDownDataRestorerRedis::DeleteExecutingInviteCmdList(const std::string& gbDownlinkerDeviceId, int workerThreadIdx)
{
	CHECK_CONDITION(started_, "please start CDownDataRestorerRedis first");
	std::lock_guard<std::mutex> guard(operationQueueMutex_);

	DataRestorerOperation operation;
	operation.operationType = DataRestorerOperation::CLEAR;
	operation.entityType = DataRestorerOperation::EXECUTING_INVITE_CMD;
	operation.gbDownlinkerDeviceId = gbDownlinkerDeviceId;
	operation.workerThreadIdx = workerThreadIdx;

	if (operationQueue_.size() > maxQueueSize_)
		operationQueue_.pop();
	operationQueue_.push(operation);
	operationQueueNotEmptyCondvar_.notify_one();
	return RESTORER_OK;
}


} // namespace GBDownLinker


