#include"devicemgr.h"

DeviceMgr::DeviceMgr(RedisClient* redis_client)
	:redis_client_(redis_client)
{
	LOG(INFO)<<"DeviceMgr constructed";
}

int DeviceMgr::LoadDevices(list<Device>& devices)
{
	list<string> device_id_list;
	redis_client_.smembers(s_key_prefix, device_id_list);
	for(list<string>::iterator it=device_id_list.begin(); it!=device_id_list.end(); it++)
	{
		Device device;
		if(redis_client_.getSerial(*it, device))
		{
			devices.insert(device);
		}
	}
	return 0;
}

int SearchDevice(string& deviceId)
{}

int InsertDevice(const Device& device)
{}

int DeleteDevice(const Device& device)
{}

int ClearDevices()
{}

int UpdateDeviceList(const list<Device>& devices)
{}


