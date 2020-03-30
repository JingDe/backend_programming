#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_REDIS_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_REDIS_H

#include "down_data_restorer_def.h"
#include "down_data_restorer.h"
#include "redisclient.h"
#include "redisbase.h"
#include "data_restorer_operation.h"

#include <string>
#include <vector>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>


namespace GBDownLinker {

class RedisClient;
class DeviceMgr;
class ChannelMgr;
class ExecutingInviteCmdMgr;
class CountDownLatch;


class CDownDataRestorerRedis : public CDownDataRestorer {

public:
	CDownDataRestorerRedis();
	~CDownDataRestorerRedis();

	void RedisClientCallback(RedisClientStatus status);

	int Init(const RestorerParamPtr& restorer_param, RestorerWorkHealthy initialRestorerWorkHealthy);
	int Uninit();
	int Start();
	int Stop();
	int GetStats();

	int PrepareLoadData(const std::string& gbdownlinker_device_id, int worker_thread_number);

	int LoadDeviceList(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* device_list);
	int InsertDeviceList(const std::string& gbdownlinker_device_id, Device* device);
	int UpdateDeviceList(const std::string& gbdownlinker_device_id, Device* device);
	int DeleteDeviceList(const std::string& gbdownlinker_device_id);
	int DeleteDeviceList(const std::string& gbdownlinker_device_id, const std::string& device_id);

	int LoadChannelList(const std::string& gbdownlinker_device_id, std::list<ChannelPtr>* channel_list);
	int InsertChannelList(const std::string& gbdownlinker_device_id, Channel* channel);
	int UpdateChannelList(const std::string& gbdownlinker_device_id, Channel* channel);
	int DeleteChannelList(const std::string& gbdownlinker_device_id);
	int DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id);
	int DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& channel_device_id);

	int LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists);
	int InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd);
	int UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd);
	int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq);
	int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx);


private:
	int Init(RedisMode redis_mode, const REDIS_SERVER_LIST& redis_server_list, const std::string& master_name, int worker_thread_num, uint32_t connect_timeout_ms, uint32_t read_timeout_ms, const std::string& passwd);

	static void DataRestorerThreadFuncWrapper(void* arg);
	void DataRestorerThreadFunc();
	void StopDataRestorerThread();

	void StopRedisClient();
	void StopRestorerMgr();
	void ClearOperationQueue();

private:
	//ErrorReportCallback errorCallback_;
	RedisMode redisMode_;
	RedisClient* redisClient_;
	bool inited_;
	bool started_;
	bool forceThreadExit_;
	std::unique_ptr<std::thread> dataRestorerThread_;

	DeviceMgr* deviceMgr_;
	ChannelMgr* channelMgr_;
	ExecutingInviteCmdMgr* inviteMgr_;

	mutable std::mutex operationQueueMutex_;
	std::condition_variable operationQueueNotEmptyCondvar_;
	std::queue<DataRestorerOperation> operationQueue_;
//	size_t maxQueueSize_;
	
	size_t unrecoverableQueueThreshold;
	RestorerWorkStatus workStatus_;
	bool needClearBackupData_;
	std::function<void(RestorerWorkHealthy)> callback_;
};

} // namespace GBDownLinker

#endif
