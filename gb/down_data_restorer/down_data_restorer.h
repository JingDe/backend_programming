#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_H

#include"down_data_restorer_def.h"
#include"device.h"
#include"channel.h"
#include"executing_invite_cmd.h"
#include"mutexlock.h"
#include"condvar.h"
#include"redisclient.h"
#include"redisbase.h"
#include"data_restorer_operation.h"

#include<cassert>
#include<vector>
#include<list>
#include<queue>
#include<pthread.h>

using std::vector;
using std::list;
using std::queue;

namespace GBGateway {

#define DDR_OK 		1
#define DDR_FAIL 	0

class RedisClient;
class DeviceMgr;
class ChannelMgr;
class ExecutingInviteCmdMgr;
class CountDownLatch;


class DownDataRestorer{
//	typedef list<ExecutingInviteCmd> ExecutingInviteCmdList;

public:
    DownDataRestorer();
	~DownDataRestorer();

	int Init(const string& redis_server_ip, uint16_t redis_server_port, int worker_thread_num, uint32_t connect_timeout_ms=900, uint32_t read_timeout_ms=3000, const string& passwd="");
    int Init(const REDIS_SERVER_LIST& redis_server_list, int worker_thread_num, uint32_t connect_timeout_ms=900, uint32_t read_timeout_ms=3000, const string& passwd="");
    int Init(const REDIS_SERVER_LIST& redis_server_list, const string& master_name, int worker_thread_num, uint32_t connect_timeout_ms=900, uint32_t read_timeout_ms=3000, const string& passwd="");
    int Uninit();
    int Start();
    int Stop();
    int GetStat();

	int LoadDeviceList(list<Device>& devices);
	int SelectDeviceList(const string& device_id, Device& device);
	int InsertDeviceList(const Device& device);
	int DeleteDeviceList(const Device& device);
	int ClearDeviceList();
	int UpdateDeviceList(const list<Device>& devices);

	int LoadChannelList(list<Channel>& channels);
	int SelectChannelList(const string& channel_id, Channel& channel);
	int InsertChannelList(const Channel& channel);
	int DeleteChannelList(const Channel& channel);
	int ClearChannelList();
	int UpdateChannelList(const list<Channel>& channels);

	int LoadExecutingInviteCmdList(vector<ExecutingInviteCmdList>& executinginvitecmdlists);
	int LoadExecutingInviteCmdList(int worker_thread_no, ExecutingInviteCmdList& executing_invite_cmd_lists);
	int SelectExecutingInviteCmdList(int worker_thread_no, const string& cmd_id, ExecutingInviteCmd& cmd);
	int InsertExecutingInviteCmdList(int worker_thread_no, const ExecutingInviteCmd& executinginvitecmd);
	int DeleteExecutingInviteCmdList(int worker_thread_no, const ExecutingInviteCmd& executinginvitecmd);
	int ClearExecutingInviteCmdList();
	int ClearExecutingInviteCmdList(int worker_thread_no);
	int UpdateExecutingInviteCmdList(int worker_thread_no, const ExecutingInviteCmdList& executinginvitecmdlist);
	int UpdateExecutingInviteCmdList(const vector<ExecutingInviteCmdList>& executinginvitecmdlists);

	int GetDeviceCount();
	int GetChannelCount();
	int GetExecutingInviteCmdCount(int worker_thread_no);

//	void SetWorkerThreadNum(int num);
	bool Inited() { return inited_; }

private:
	int Init(RedisMode redis_mode, const REDIS_SERVER_LIST& redis_server_list, const string& master_name, int worker_thread_num, uint32_t connect_timeout_ms, uint32_t read_timeout_ms, const string& passwd);
    static void* DataRestorerThreadFuncWrapper(void* arg);
    void DataRestorerThreadFunc();
	int GetWorkerThreadNum();
	void KillDataRestorerThread();
	
	RedisClient* redis_client_;
	bool inited_;
	bool started_;
    bool force_thread_exit_;	
	int data_restorer_background_threads_num_; // same as RedisClient m_connectionNum
	vector<pthread_t> data_restorer_threads_;
	CountDownLatch* data_restorer_threads_exit_latch;
	
	DeviceMgr* device_mgr_;
	ChannelMgr* channel_mgr_;
	ExecutingInviteCmdMgr* executing_invite_cmd_mgr_;

	MutexLock operations_queue_mutex_;
	CondVar operations_queue_not_empty_condvar;
	queue<DataRestorerOperation> operations_queue_;

	int worker_thread_num_;
    RedisMode redis_mode_;
};

} // namespace GBGateway

#endif
