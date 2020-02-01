#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_H

#include"device.h"
#include"channel.h"
#include"executinginvitecmd.h"

#include<vector>
#include<list>
#include<pthread.h>


using std::vector;
using std::list;

class RedisClient;

#define DDR_OK 		0
#define DDR_FAIL 	-1

class DownDataRestorer{
	typedef list<ExecutingInviteCmd> ExecutingInviteCmdList;

public:
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
	int InsertExecutingInviteCmdList(const ExecutingInviteCmd& executinginvitecmd);
	int DeleteExecutingInviteCmdList(const ExecutingInviteCmd& executinginvitecmd);
	int ClearExecutingInviteCmdList();
	// TODO
	int UpdateExecutingInviteCmdList(const ExecutingInviteCmdList& executinginvitecmdlist);
	int UpdateExecutingInviteCmdList(const vector<ExecutingInviteCmdList>& executinginvitecmdlists);

private:
	RedisClient* redis_client_;
	bool inited_;
	bool started_;
	int data_restorer_threads_num_; // same as RedisClient m_connectionNum
	vector<pthread_t> data_restorer_threads_;
	CountDownLatch* data_restorer_threads_exit_latch;
	DeviceMgr* device_mgr_;
	ChannelMgr* channel_mgr_;
	ExecutingInviteCmdMgr* executing_invite_cmd_mgr_;
};

#endif
