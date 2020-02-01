#include"redisclient.h"
#include<string>

using std::string;

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


int main(int argc, char** argv)
{
    string log_config_filename="log.conf";
    if(argc<2)
    {
        printf("usage: %s configFileName\n", argv[0]);
        prnitf("default: %s", log_config_filename.c_str());
//        return -1;
    }
    log_config_filename=argv[1];
    printf("log config filename %s\n", log_config_filename.c_str());

    OWLog::config(log_config_filename);

	TestTransactionSetDel();
	
    return 0;
}
