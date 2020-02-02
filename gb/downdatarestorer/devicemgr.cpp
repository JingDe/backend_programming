#include"devicemgr.h"
#include"glog/logging.h"

const string DeviceMgr::s_set_key="deviceset";
const string DeviceMgr::s_key_prefix="deviceset";

DeviceMgr::DeviceMgr(RedisClient* redis_client)
	:redis_client_(redis_client)
{
	LOG(INFO)<<"DeviceMgr constructed";
}

int DeviceMgr::LoadDevices(list<Device>& devices)
{
	time_t now=time(NULL);
	list<string> device_id_list;
	redis_client_.smembers(s_set_key, device_id_list);
	for(list<string>::iterator it=device_id_list.begin(); it!=device_id_list.end(); it++)
	{
		Device device;
		if(redis_client_.getSerial(s_key_prefix+*it, device))
		{
			// update RegisterLastTime, KeepaliveLastTime
			device.UpdateRegisterLastTime(now);
			device.UpdateKeepaliveLastTime(now);
			devices.insert(device);
		}
	}
	return 0;
}

int DeviceMgr::SearchDevice(string& deviceId, Device& device)
{
	list<string> device_id_list;
	bool exist=redis_client_.sismember(s_set_key, deviceId);
	if(exist)
	{
		if(redis_client_.getSerial(s_key_prefix+*it, device))
		{
			return 0;
		}
	}	
	return -1;
}

int DeviceMgr::InsertDevice(const Device& device)
{
    RedisConnection *con=NULL;

    if(!redis_client_.PrepareTransaction(&con))
    {
    	LOG(ERROR)<<"PrepareTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_.StartTransaction(con))
    {
	    LOG(ERROR)<<"StartTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_.Set(con, s_key_prefix+device.getDeviceId(), device))
    {
	    LOG(ERROR)<<"Set failed";
    	goto FAIL;
    }
    if(!redis_client_.Sadd(con, s_set_key, device.getDeviceId()))
    {
	    LOG(ERROR)<<"Sadd failed";
    	goto FAIL;
    }
    if(!redis_client_.ExecTransaction(con))
    {
	    LOG(ERROR)<<"ExecTransaction failed";
    	goto FAIL;
    }
            
    redis_client_.FinishTransaction(&con);
    LOG(ERROR)<<"InsertDevice success: "<<device.getDeviceId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_.FinishTransaction(&con);
    }
    LOG(ERROR)<<"InsertDevice failed: "<<device.getDeviceId();
    return -1;
}

int DeviceMgr::DeleteDevice(const Device& device)
{
	RedisConnection *con=NULL;

    if(!redis_client_.PrepareTransaction(&con))
    {
    	LOG(ERROR)<<"PrepareTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_.StartTransaction(con))
    {
	    LOG(ERROR)<<"StartTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_.Del(con, s_key_prefix+device.getDeviceId()))
    {
	    LOG(ERROR)<<"Del failed";
    	goto FAIL;
    }
    if(!redis_client_.Srem(con, s_set_key, device.getDeviceId()))
    {
	    LOG(ERROR)<<"Srem failed";
    	goto FAIL;
    }
    if(!redis_client_.ExecTransaction(con))
    {
	    LOG(ERROR)<<"ExecTransaction failed";
    	goto FAIL;
    }
            
    redis_client_.FinishTransaction(&con);
    LOG(ERROR)<<"DeleteDevice success: "<<device.getDeviceId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_.FinishTransaction(&con);
    }
    LOG(ERROR)<<"DeleteDevice failed: "<<device.getDeviceId();
    return -1;
}

int DeviceMgr::ClearDevices()
{
	// TODO lock
	list<string> device_id_list;
	redis_client_.smembers(s_set_key, device_id_list);
	redis_client_.del(s_set_key);
	for(list<string>::iterator it=device_id_list.begin(); it!=device_id_list.end(); it++)
	{
		redis_client_.del(s_key_prefix+*it);
	}
	return 0;
}

int DeviceMgr::UpdateDevices(const list<Device>& devices)
{
	// TODO lock
	for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		redis_client_.sadd(s_set_key, it->getDeviceId());
		redis_client_.setSerial(s_key_prefix+it->getDeviceId(), *it);
	}
}


