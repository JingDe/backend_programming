#ifndef DOWN_DATA_RESTORER_DEVICE_MGR_H
#define DOWN_DATA_RESTORER_DEVICE_MGR_H

#include"device.h"

class RedisClient;

class DeviceMgr{
public:
	DeviceMgr(RedisClient*);

	int LoadDevices(list<Device>& devices);
	int SearchDevice(string& deviceId);
	int InsertDevice(const Device& device);
	int DeleteDevice(const Device& device);
	int ClearDevices();
	int UpdateDeviceList(const list<Device>& devices);

private:
	RedisClient* redis_client_;
};

#endif