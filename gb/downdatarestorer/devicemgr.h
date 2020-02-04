#ifndef DOWN_DATA_RESTORER_DEVICE_MGR_H
#define DOWN_DATA_RESTORER_DEVICE_MGR_H

#include"device.h"
#include<list>
#include<string>

using std::list;
using std::string;

class RedisClient;

class DeviceMgr{
public:
	DeviceMgr(RedisClient*);

	int LoadDevices(list<Device>& devices);
	int SearchDevice(const string& device_id, Device& device);
	int InsertDevice(const Device& device);
	int DeleteDevice(const Device& device);
	int ClearDevices();
	int UpdateDevices(const list<Device>& devices);
	int UpdateDevices(const list<void*>& devices);

private:
	RedisClient* redis_client_;

	const static string s_set_key;
	const static string s_key_prefix;
};

#endif