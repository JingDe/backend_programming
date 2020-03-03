#include"device_mgr.h"
#include"redisclient.h"
#include"mutexlockguard.h"
#include "base_library/log.h"
#include"serialize_entity.h"
#include<sstream>

namespace GBGateway {

const string DeviceMgr::s_set_key="gbddrdeviceset";
const string DeviceMgr::s_key_prefix="gbddrdeviceset";

DeviceMgr::DeviceMgr(RedisClient* redis_client, RedisMode redis_mode)
	:redis_client_(redis_client),
	redis_mode_(redis_mode),
//	modify_mutex_(),
	rwmutex_()
{
	LOG_WRITE_INFO("DeviceMgr constructed");
}

int DeviceMgr::LoadDevices(list<Device>& devices)
{
	ReadGuard guard(rwmutex_);
	
//	time_t now=time(NULL);
	list<string> device_id_list;
	redis_client_->smembers(s_set_key, device_id_list);
	for(list<string>::iterator it=device_id_list.begin(); it!=device_id_list.end(); it++)
	{
		Device device;
		if(redis_client_->getSerial(s_key_prefix+*it, device))
		{
//			device.UpdateRegisterLastTime(now);
//			device.UpdateKeepaliveLastTime(now);
			device.registerLastTime=base::CTime::GetCurrentTime();
			device.keepaliveLastTime=base::CTime::GetCurrentTime();
			devices.push_back(device);
		}
	}
	return 0;
}

int DeviceMgr::SearchDevice(const string& device_id, Device& device)
{
	ReadGuard guard(rwmutex_);
	
	bool exist=redis_client_->sismember(s_set_key, device_id);
	if(exist)
	{
		if(redis_client_->getSerial(s_key_prefix+device_id, device))
		{
			return 0;
		}
	}	
	return -1;
}

int DeviceMgr::InsertDevice(const Device& device)
{
	if(redis_mode_==STAND_ALONE_OR_PROXY_MODE)
		return InsertDeviceInTransaction(device);
	else //if(redis_mode_==CLUSTER_MODE)
		return InsertDeviceInCluster(device);
}

int DeviceMgr::InsertDeviceInCluster(const Device& device)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	if(redis_client_->setSerial(s_key_prefix+device.deviceId, device)==false)
	{
		return -1;
	}
	
	if(redis_client_->sadd(s_set_key, device.deviceId)==true)
	{
		return 0;
	}
	else
	{
		if(redis_client_->del(s_key_prefix+device.deviceId)==false)
		{
			std::stringstream log_msg;
			log_msg<<"insert device"<<device.deviceId<<" failed, undo set failed";
			LOG_WRITE_ERROR(log_msg.str());
		}
		return -1;
	}
}

int DeviceMgr::InsertDeviceInTransaction(const Device& device)
{
    //     MutexLockGuard guard(&modify_mutex_);
    WriteGuard guard(rwmutex_);

    std::stringstream log_msg;
    RedisConnection *con=NULL;

    if(!redis_client_->PrepareTransaction(&con))
    {
        goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
        goto FAIL;
    }
    if(!redis_client_->Set(con, s_key_prefix+device.deviceId, device))
    {
        goto FAIL;
    }
    if(!redis_client_->Sadd(con, s_set_key, device.deviceId))
    {
        goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
        goto FAIL;
    }
    redis_client_->FinishTransaction(&con);
	log_msg.str("");
	log_msg<<"InsertDevice success: "<<device.deviceId;
	LOG_WRITE_INFO(log_msg.str());
    return 0;

FAIL:
    if(con)
    {
        redis_client_->FinishTransaction(&con);
    }
	log_msg.str("");
	log_msg<<"InsertDevice failed: "<<device.deviceId;
	LOG_WRITE_ERROR(log_msg.str());
    return -1;
}

int DeviceMgr::DeleteDevice(const Device& device)
{
	if(redis_mode_==STAND_ALONE_OR_PROXY_MODE)
		return DeleteDeviceInTransaction(device);
	else //if(redis_mode_==CLUSTER_MODE)
		return DeleteDeviceInCluster(device);
}

int DeviceMgr::DeleteDeviceInCluster(const Device& device)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	if(redis_client_->del(s_key_prefix+device.deviceId)==false)
		return -1;
	
	if(redis_client_->srem(s_set_key, device.deviceId)==true)
	{
		std::stringstream log_msg;
		log_msg<<"delete device "<<device.deviceId<<" ok";
		LOG_WRITE_INFO(log_msg.str());
		return 0;
	}
	else
	{
		if(redis_client_->setSerial(s_key_prefix+device.deviceId, device)==false)
		{
			std::stringstream log_msg;
			log_msg<<"delete device"<<device.deviceId<<" failed, undo del failed";
			LOG_WRITE_ERROR(log_msg.str());
		}
		return -1;
	}
}


int DeviceMgr::DeleteDeviceInTransaction(const Device& device)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

    std::stringstream log_msg;
	RedisConnection *con=NULL;

    if(!redis_client_->PrepareTransaction(&con))
    {
    	goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
    	goto FAIL;
    }
    if(!redis_client_->Del(con, s_key_prefix+device.deviceId))
    {
    	goto FAIL;
    }
    if(!redis_client_->Srem(con, s_set_key, device.deviceId))
    {
    	goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
    	goto FAIL;
    }
            
    redis_client_->FinishTransaction(&con);
	log_msg.str("");
	log_msg<<"DeleteDevice success: "<<device.deviceId;
	LOG_WRITE_INFO(log_msg.str());
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
	log_msg.str("");
	log_msg<<"DeleteDevice failed: "<<device.deviceId;
	LOG_WRITE_ERROR(log_msg.str());
    return -1;
}

int DeviceMgr::ClearDevices()
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);
	
	return ClearDevicesWithLockHeld();
}

int DeviceMgr::ClearDevicesWithLockHeld()
{	
	list<string> device_id_list;
	redis_client_->smembers(s_set_key, device_id_list);
	redis_client_->del(s_set_key);
	for(list<string>::iterator it=device_id_list.begin(); it!=device_id_list.end(); it++)
	{
		redis_client_->del(s_key_prefix+*it);
	}
	return 0;
}



int DeviceMgr::UpdateDevices(const list<Device>& devices)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	ClearDevicesWithLockHeld();
	
	for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		redis_client_->sadd(s_set_key, it->deviceId);
		redis_client_->setSerial(s_key_prefix+it->deviceId, *it);
	}
    return 0;
}

int DeviceMgr::UpdateDevices(const list<void*>& devices)
{
//	MutexLockGuard guard(&modify_mutex_);
	WriteGuard guard(rwmutex_);

	ClearDevicesWithLockHeld();

	for(list<void*>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		Device* device=(Device*)(*it);
		redis_client_->sadd(s_set_key, device->deviceId);
		redis_client_->setSerial(s_key_prefix+device->deviceId, *device);
	}
    return 0;
}

int DeviceMgr::GetDeviceCount()
{
	ReadGuard guard(rwmutex_);
	return redis_client_->scard(s_set_key);
}

} // namespace GBGateway
