#include"device_mgr.h"
#include"redisclient.h"
#include"mutexlockguard.h"
#include"base_library/log.h"
#include"serialize_entity.h"
#include<sstream>

namespace GBGateway {


DeviceMgr::DeviceMgr(RedisClient* redis_client)
	:redis_client_(redis_client),
	rwmutex_()
{
	LOG_WRITE_INFO("DeviceMgr constructed");
}

int DeviceMgr::GetDeviceKeyList(std::string gbdownlinker_device_id, std::list<std::string>& device_key_list)
{
	device_key_list.clear();
	std::string key_prefix = std::string("{downlinker.device}:") + gbdownlinker_device_id;
	redis_client_->getKeys(key_prefix, device_key_list);
	return 0;
}

int DeviceMgr::LoadDevice(const std::list<std::string>& device_key_list, std::list<DevicePtr>* devices)
{
	devices.clear();
	for (std::list<std::string>::const_iterator cit = device_key_list.cbegin(); cit != device_key_list.cend(); cit++)
	{
		Device* device=new Device();
		if (!redis_client_->getSerial(*it, *device))
			continue;
		device->registerLastTime = base::CTime::GetCurrentTime();
		device->keepaliveLastTime = base::CTime::GetCurrentTime();

		DevicePtr device_ptr(device);
		devices.push_back(device_ptr);
	}
	return 0;
}

int DeviceMgr::LoadDevice(std::string gbdownlinker_device_id, std::list<DevicePtr>* devices)
{
	std::list<std::string> device_key_list;
	GetDeviceKeyList(gbdownlinker_device_id, device_key_list);
	LoadDevices(device_key_list, devices);
	return 0;
}


int DeviceMgr::InsertDevice(const std::string& gbdownlinker_device_id, const Device& device)
{
	WriteGuard guard(rwmutex_);

	std::string device_key = std::string("{downlinker.device}:") + gbdownlinker_device_id + ":" + device.deviceId;
	if(redis_client_->setSerial(device_key, device)==false)
	{
		return -1;
	}
	return 0;
}

int DeviceMgr::InsertDevice(const std::string& gbdownlinker_device_id, const Device& device)
{
	return InsertDevice(gbdownlinker_device_id, device);
}

int DeviceMgr::DeleteDevice(const std::string& gbdownlinker_device_id, const std::string& device_id)
{
	WriteGuard guard(rwmutex_);

	std::string device_key = std::string("{downlinker.device}:") + gbdownlinker_device_id + ":" + device_id;
	if(redis_client_->del(device_key)==false)
		return -1;
	
	return 0;
}

int DeviceMgr::ClearDevice(const std::string& gbdownlinker_device_id)
{
	WriteGuard guard(rwmutex_);

	std::list<std::string> device_key_list;
	GetDeviceKeyList(gbdownlinker_device_id, device_key_list);

	for(std::list<std::string>::iterator it= device_key_list.begin(); it!= device_key_list.end(); it++)
	{
		redis_client_->del(*it);
	}
	return 0;
}

int DeviceMgr::GetDeviceCount(const std::string& gbdownlinker_device_id)
{
	ReadGuard guard(rwmutex_);
	std::list<std::string> device_key_list;
	GetDeviceKeyList(gbdownlinker_device_id, device_key_list);
	return device_key_list.size();
}

} // namespace GBGateway
