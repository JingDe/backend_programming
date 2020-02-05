#include"redisclient.h"
#include"downdatarestorer.h"
#include"device.h"
#include"glog/logging.h"
#include<string>
#include<list>
#include<iostream>
#include<cstdio>

using std::string;
using std::list;


const string redis_ip="192.168.12.59";
const uint16_t redis_port=6379;

bool TestTransactionSetAdd()
{
    RedisClient client;
    printf("construct RedisClient ok\n");

	RedisServerInfo server(redis_ip, redis_port);
	REDIS_SERVER_LIST servers({server});
	uint32_t conn_num=10;
	uint32_t keepalive_time=60;
	if(client.init(servers, conn_num, keepalive_time)==false)
	{
		printf("client init failed\n");
		return false;
	}
	
	RedisConnection *con=NULL;
	do{
	    if(!client.PrepareTransaction(&con))
	    	break;
	    	
	    if(!client.StartTransaction(con))
	    	break;
	    if(!client.Set(con, "GbDeviceSet_deviceid1", "deviceid1desc"))
	    	break;
	    if(!client.Sadd(con, "GbDeviceSet", "GbDeviceSet_deviceid1"))
	    	break;
	    if(!client.ExecTransaction(con))
	    	break;
	    	
	    client.FinishTransaction(&con);
	    printf("success\n");
	    		    
    }while(0);

    if(con)
    {
    	printf("failed\n");
    	client.FinishTransaction(&con);
    }

    return true;
}

bool TestTransactionSetDel()
{
    RedisClient client;
    printf("construct RedisClient ok\n");

	RedisServerInfo server(redis_ip, redis_port);
	REDIS_SERVER_LIST servers({server});
	uint32_t conn_num=10;
	uint32_t keepalive_time=60;
	if(client.init(servers, conn_num, keepalive_time)==false)
	{
		printf("client init failed\n");
		return false;
	}
	
	RedisConnection *con=NULL;
	do{
	    if(!client.PrepareTransaction(&con))
	    	break;
	    	
	    if(!client.StartTransaction(con))
	    	break;
	    if(!client.Srem(con, "GbDeviceSet", "GbDeviceSet_deviceid1"))
	    	break;
	    if(!client.Del(con, "GbDeviceSet_deviceid1"))
	    	break;
	    if(!client.ExecTransaction(con))
	    	break;
	    	
	    client.FinishTransaction(&con);
	    printf("success\n");
	    		    
    }while(0);

    if(con)
    {
    	printf("failed\n");
    	client.FinishTransaction(&con);
    }

    return true;
}

void print(const list<Device>& devices)
{
	for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		printf("deviceId=%s, ", it->GetDeviceId().c_str());
	}
	printf("\n");
}


void TestDownDataRestorer()
{
	DownDataRestorer ddr;
	ddr.Init(redis_ip, redis_port, 8);
	ddr.Start();

	
	std::cout<<"devices size: "<<ddr.GetDeviceCount()<<std::endl;

	Device device1("device_id_1");
	ddr.InsertDeviceList(device1);

	std::cout<<"devices size: "<<ddr.GetDeviceCount()<<std::endl;
	list<Device> devices;
	ddr.LoadDeviceList(devices);
	print(devices);

	ddr.DeleteDeviceList(device1);
	std::cout<<"devices size: "<<ddr.GetDeviceCount()<<std::endl;

	ddr.Stop();
	ddr.Uninit();
	
}

int main(int argc, char** argv)
{
    string log_config_filename="log.conf";
    if(argc<2)
    {
        printf("usage: %s configFileName\n", argv[0]);
        printf("default: %s", log_config_filename.c_str());
    }
    else
    {
        log_config_filename=argv[1];
        printf("log config filename %s\n", log_config_filename.c_str());
    }

    OWLog::config(log_config_filename);

    TestDownDataRestorer();
    return 0;
}
