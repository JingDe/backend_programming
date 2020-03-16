#ifndef DOWN_DATA_RESTORER_DEVICE_MGR_H
#define DOWN_DATA_RESTORER_DEVICE_MGR_H

#include"device.h"
#include"rwmutex.h"
#include"redisbase.h"
#include<list>
#include<string>

// {downlinker.device}:gbdownlinker_device_id:device_id

namespace GBGateway {

class RedisClient;

class DeviceMgr{
public:
	DeviceMgr(RedisClient*);
	
	int GetDeviceKeyList(const std::string& gbdownlinker_device_id, std::list<std::string>& device_key_list);

	int LoadDevice(const std::list<std::string>& device_key_list, std::list<DevicePtr>* devices);
	int LoadDevice(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices);

	int InsertDevice(const std::string& gbdownlinker_device_id, const Device& device);
	int UpdateDevice(const std::string& gbdownlinker_device_id, const Device& devices);
	int DeleteDevice(const std::string& gbdownlinker_device_id, const std::string& device_id);
	int ClearDevice(const std::string& gbdownlinker_device_id);

	size_t GetDeviceCount(const std::string& gbdownlinker_device_id);

private:
	int InsertDeviceInTransaction(const Device& device);
	int InsertDeviceInCluster(const Device& device);
	int DeleteDeviceInTransaction(const Device& device);
	int DeleteDeviceInCluster(const Device& device);
	int ClearDeviceWithLockHeld();

	RedisClient* redis_client_;
	RWMutex rwmutex_;
};

} // namespace GBGateway

#endif
