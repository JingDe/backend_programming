#ifndef DOWN_DATA_RESTORER_DEVICE_MGR_H
#define DOWN_DATA_RESTORER_DEVICE_MGR_H

#include"device.h"
//#include"mutexlock.h"
#include"rwmutex.h"
//#include"owlog.h"
#include"downdatarestorerdef.h"
#include<list>
#include<string>

using std::list;
using std::string;

class RedisClient;

class DeviceMgr{
public:
	DeviceMgr(RedisClient*, RedisMode);

	int LoadDevices(list<Device>& devices);
	int SearchDevice(const string& device_id, Device& device);
	int InsertDevice(const Device& device);
	int DeleteDevice(const Device& device);
	int ClearDevices();	
	int UpdateDevices(const list<Device>& devices);
	int UpdateDevices(const list<void*>& devices);
	int GetDeviceCount();

private:
	int InsertDeviceInTransaction(const Device& device);
	int InsertDeviceInCluster(const Device& device);
	int DeleteDeviceInTransaction(const Device& device);
	int DeleteDeviceInCluster(const Device& device);
	int ClearDevicesWithLockHeld();

	RedisClient* redis_client_;
	RedisMode redis_mode_;
//	MutexLock modify_mutex_;
	RWMutex rwmutex_;
//    OWLog logger_;
	const static string s_set_key;
	const static string s_key_prefix;
};

#endif
