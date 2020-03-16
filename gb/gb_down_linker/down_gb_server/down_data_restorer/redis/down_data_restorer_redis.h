#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_REDIS_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_REDIS_H

#include"down_data_restorer_def.h"
#include"down_data_restorer.h"
//#include"device.h"
//#include"channel.h"
//#include"executing_invite_cmd.h"
#include"mutexlock.h"
#include"condvar.h"
#include"redisclient.h"
#include"redisbase.h"
#include"data_restorer_operation.h"

#include<string>
#include<vector>
#include<list>
#include<queue>
#include<pthread.h>


namespace GBGateway {

#define RESTORER_OK 		1
#define RESTORER_FAIL 		0

class RedisClient;
class DeviceMgr;
class ChannelMgr;
class ExecutingInviteCmdMgr;
class CountDownLatch;


class CDownDataRestorerRedis : public CDownDataRestorer {

public:
    CDownDataRestorerRedis();
	~CDownDataRestorerRedis();

	int Init(RestorerParamPtr restorer_param);
    int Uninit();
    int Start();
    int Stop();
    int GetStat(const std::string& gbdownlinker_device_id, int worker_thread_idx);

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
	int DeleteChannelList(const std::string& gbdownlinker_device_id, const std::string& device_id, const std::string& device_channel_id);

	int LoadExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, std::list<ExecutingInviteCmdPtr>* executing_invite_cmd_lists);
	int InsertExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd);
	int UpdateExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, ExecutingInviteCmd* executinginvitecmd);
	int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx, const std::string& cmd_sender_id, const std::string& device_id, const int64_t& cmd_seq);
	int DeleteExecutingInviteCmdList(const std::string& gbdownlinker_device_id, int worker_thread_idx);
	

private:	
	int GetDeviceCount(const std::string& gbdownlinker_device_id);
	int GetChannelCount(const std::string& gbdownlinker_device_id);
	int GetExecutingInviteCmdCount(const std::string& gbdownlinker_device_id, int worker_thread_idx);

	bool Inited() { return inited_; }
	int Init(RedisMode redis_mode, const REDIS_SERVER_LIST& redis_server_list, const std::string& master_name, int worker_thread_num, uint32_t connect_timeout_ms, uint32_t read_timeout_ms, const std::string& passwd);

    static void* DataRestorerThreadFuncWrapper(void* arg);
    void DataRestorerThreadFunc();
	void KillDataRestorerThread();

private:
	//ErrorReportCallback error_callback_;
	//int worker_thread_num_;
	RedisMode redis_mode_;
	RedisClient* redis_client_;
	bool inited_;
	bool started_;
    bool force_thread_exit_;	
	int data_restorer_background_threads_num_; // same as RedisClient m_connectionNum
	std::vector<pthread_t> data_restorer_threads_;
	CountDownLatch* data_restorer_threads_exit_latch;
	
	DeviceMgr* device_mgr_;
	ChannelMgr* channel_mgr_;
	ExecutingInviteCmdMgr* executing_invite_cmd_mgr_;

	MutexLock operations_queue_mutex_;
	CondVar operations_queue_not_empty_condvar;
	std::queue<DataRestorerOperation> operations_queue_;
    
	std::string gbdownlinker_device_id_;
	std::list<std::string>& device_key_list_;
	std::list<std::string>& channel_key_list_;
	std::list<std::string>& invite_key_list_;
	std::vector<std::list<std::string> > invite_key_lists_;
};

} // namespace GBGateway

#endif
