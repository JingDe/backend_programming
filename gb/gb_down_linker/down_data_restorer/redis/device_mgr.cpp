#include "device_mgr.h"
#include "redisclient.h"
#include "mutexlockguard.h"
#include "base_library/log.h"
#include "serialize_entity.h"
#include <sstream>

namespace GBDownLinker {


DeviceMgr::DeviceMgr(RedisClient* redis_client)
	:redisClient_(redis_client)
{
	LOG_WRITE_INFO("DeviceMgr constructed");
}

int DeviceMgr::GetDeviceKeyList(const std::string& gbdownlinker_device_id, std::list<std::string>& device_key_list)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	device_key_list.clear();
	std::string key_prefix = std::string("{downlinker.device}:") + gbdownlinker_device_id;
	redisClient_->getKeys(key_prefix, device_key_list);
	return 0;
}

int DeviceMgr::LoadDevice(const std::list<std::string>& device_key_list, std::list<DevicePtr>* devices)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	devices->clear();
	for (std::list<std::string>::const_iterator cit = device_key_list.cbegin(); cit != device_key_list.cend(); cit++)
	{
		Device* device = new Device();
		if (!redisClient_->getSerial(*cit, *device))
			continue;
		device->registerLastTime = base::CTime::GetCurrentTime();
		device->keepaliveLastTime = base::CTime::GetCurrentTime();

		DevicePtr device_ptr(device);
		devices->push_back(std::move(device_ptr));
	}
	return 0;
}

int DeviceMgr::LoadDevice(const std::string& gbdownlinker_device_id, std::list<DevicePtr>* devices)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	std::list<std::string> device_key_list;
	GetDeviceKeyList(gbdownlinker_device_id, device_key_list);
	LoadDevice(device_key_list, devices);
	return 0;
}


int DeviceMgr::InsertDevice(const std::string& gbdownlinker_device_id, const Device& device)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);
	
	std::string device_key = std::string("{downlinker.device}:") + gbdownlinker_device_id + std::string(":") + device.deviceId;

	if (redisClient_->setSerial(device_key, device) == false)
	{
		return -1;
	}
	return 0;
}

int DeviceMgr::UpdateDevice(const std::string& gbdownlinker_device_id, const Device& device)
{
	return InsertDevice(gbdownlinker_device_id, device);
}

int DeviceMgr::DeleteDevice(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	std::string device_key = std::string("{downlinker.device}:") + gbdownlinker_device_id + ":" + device_id;
	if (redisClient_->del(device_key) == false)
		return -1;

	return 0;
}

int DeviceMgr::ClearDevice(const std::string& gbdownlinker_device_id)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	std::list<std::string> device_key_list;
	GetDeviceKeyList(gbdownlinker_device_id, device_key_list);

	for (std::list<std::string>::iterator it = device_key_list.begin(); it != device_key_list.end(); it++)
	{
		redisClient_->del(*it);
	}
	return 0;
}

size_t DeviceMgr::GetDeviceCount(const std::string& gbdownlinker_device_id)
{
	CHECK_LOG_RETURN(redisClient_!=NULL, "RedisClient not inited", -1);

	std::list<std::string> device_key_list;
	GetDeviceKeyList(gbdownlinker_device_id, device_key_list);
	return device_key_list.size();
}

} // namespace GBDownLinker
