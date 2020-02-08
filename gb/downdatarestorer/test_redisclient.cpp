#include"redisclient.h"
#include"downdatarestorer.h"
#include"device.h"
#include"glog/logging.h"
#include"owlog.h"
#include<string>
#include<list>
#include<iostream>
#include<cstdio>
#include<unistd.h>

using std::string;
using std::list;


const string redis_ip="192.168.12.59";
const uint16_t redis_port=6379;

DownDataRestorer* g_ddr=NULL;


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
	printf("use connection %p\n", con);
	
	do{
	    if(!client.PrepareTransaction(&con))
	    	break;
	    	
	    printf("use connection %p\n", con);
	    if(!client.StartTransaction(con))
	    	break;
	    	
	    printf("use connection %p\n", con);
	    if(!client.Set(con, "GbDeviceSet_deviceid1", "deviceid1desc"))
	    	break;
	    	
	    printf("use connection %p\n", con);
	    if(!client.Sadd(con, "GbDeviceSet", "GbDeviceSet_deviceid1"))
	    	break;

	    printf("use connection %p\n", con);
	    if(!client.ExecTransaction(con))
	    	break;

	    printf("use connection %p\n", con);
	    client.FinishTransaction(&con);
	    printf("success\n");
	    		    
    }while(0);

    if(con)
    {
    	printf("failed\n");
    	printf("use connection %p\n", con);
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
	printf("print devices: [%d]\n", (int)devices.size());
	for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		printf("deviceId=%s, ", it->GetDeviceId().c_str());
	}
	printf("\n");
}

void ShowDevices(DownDataRestorer& ddr)
{
	list<Device> devices;
	ddr.LoadDeviceList(devices);
	print(devices);
}

void TestDownDataRestorerDevice()
{
    OWLog logger("common.test");
	DownDataRestorer ddr;
	ddr.Init(redis_ip, redis_port, 8);
	ddr.Start();

	int device_count=ddr.GetDeviceCount();
	logger.debug("devices size: %d", device_count);
	std::cout<<"devices size: "<<device_count<<std::endl;

	Device device1("device_id_1");
	ddr.InsertDeviceList(device1);

	sleep(5);
	
	device_count=ddr.GetDeviceCount();
	logger.debug("after insert, devices size: %d", device_count);
	std::cout<<"after insert, devices size: "<<device_count<<std::endl;

	printf("after insert, before search, to check redis-cli\n");
	getchar();
	//getc(stdin);

	Device device;
	if(ddr.SelectDeviceList("device_id_1", device)==DDR_OK)
	{
		printf("search device_id_1 success\n");
	}
	else
	{
		printf("search device_id_1 failed\n");
	}
	getchar();

	Device device2("device_id_2");
	ddr.InsertDeviceList(device2);

	Device device3("device_id_3");
	ddr.InsertDeviceList(device3);

	Device device4("device_id_4");
	ddr.InsertDeviceList(device4);
	
	ShowDevices(ddr);

	ddr.DeleteDeviceList(device1);

	sleep(5);
	ShowDevices(ddr);

	ddr.ClearDeviceList();
	sleep(5);
	
	device_count=ddr.GetDeviceCount();
	logger.debug("after clear, devices size: %d", device_count);
	std::cout<<"after clear, devices size: "<<device_count<<std::endl;
	ShowDevices(ddr);

	list<Device> devices({device1, device3});
	ddr.UpdateDeviceList(devices);

	sleep(5);
	printf("after update\n");
	ShowDevices(ddr);

	ddr.Stop();
	ddr.Uninit();
}

void PrintCmdList(const ExecutingInviteCmdList& cmdlist, int set_no)
{
	printf("start show the %d cmd set, [%d]\n", set_no, (int)cmdlist.size());
	for(ExecutingInviteCmdList::const_iterator it=cmdlist.begin(); it!=cmdlist.end(); ++it)
	{
		printf("%s,\t", it->GetId().c_str());
	}
	printf("\n");
}

void PrintCmdLists(const vector<ExecutingInviteCmdList>& cmdlists)
{
	printf("*** start show cmd lists: [%d]\n", (int)cmdlists.size());
	for(size_t i=0; i<cmdlists.size(); ++i)
	{
		PrintCmdList(cmdlists[i], i+1);
	}
	printf("*** end\n");
}

typedef void* WorkerThreadFuncType(void*);

pthread_t StartWorkerThread(int& thread_no, WorkerThreadFuncType func)
{
	pthread_t thread_id;
	printf("to start worker thread, %d, %p\n", thread_no, &thread_no);
	pthread_create(&thread_id, NULL, func, &thread_no);

	return thread_id;
}

void ShowCmdList(int thread_no)
{
	ExecutingInviteCmdList cmdlist;
	g_ddr->LoadExecutingInviteCmdList(thread_no, cmdlist);
	PrintCmdList(cmdlist, thread_no);
}

void* WorkerThread0Func(void* arg)
{
	printf("in worker, %p\n", arg);
	int thread_no=*(reinterpret_cast<int*>(arg));
	printf("%d\n", thread_no);
	printf("worker thread %d start\n", thread_no);

	ShowCmdList(thread_no);

	ExecutingInviteCmd cmd1(1);
	g_ddr->InsertExecutingInviteCmdList(thread_no, cmd1);

	ExecutingInviteCmd cmd2(2);
	g_ddr->InsertExecutingInviteCmdList(thread_no, cmd2);

	sleep(3);
	printf("after insert\n");
	ShowCmdList(thread_no);
	
	getchar();

	ExecutingInviteCmd cmd;
	if(g_ddr->SelectExecutingInviteCmdList(thread_no, "1", cmd)==DDR_OK)
	{
		printf("select ok: %s\n", cmd.GetId().c_str());
	}
	else
	{
		printf("select failed\n");
	}

	getchar();
	
	g_ddr->DeleteExecutingInviteCmdList(thread_no, cmd1);

	sleep(3);
	printf("after delete\n");
	ShowCmdList(thread_no);

	getchar();

	ExecutingInviteCmdList cmd_list({cmd1});
	g_ddr->UpdateExecutingInviteCmdList(thread_no, cmd_list);

	sleep(3);
	printf("after update\n");
	ShowCmdList(thread_no);

	printf("before clear\n");
	getchar();

	g_ddr->ClearExecutingInviteCmdList(thread_no);

	sleep(5);
	printf("after clear\n");
	ShowCmdList(thread_no);

	printf("worker thread %d exit\n", thread_no);	

	return 0;
}

void* WorkerThread1Func(void* arg)
{
	

	return 0;
}


void TestDownDataRestorerExecutingInviteCmd()
{
	DownDataRestorer ddr;
	ddr.Init(redis_ip, redis_port, 8);
	ddr.Start();

	g_ddr=&ddr;

	int worker_thread_no_0=0;
	printf("in main, %d, %p\n", worker_thread_no_0, &worker_thread_no_0);
	pthread_t t0=StartWorkerThread(worker_thread_no_0, WorkerThread0Func);
	int worker_thread_no_1=1;
//	pthread_t t1=StartWorkerThread(worker_thread_no_1, WorkerThread1Func);


	sleep(5);
	vector<ExecutingInviteCmdList> cmdlists;
	ddr.LoadExecutingInviteCmdList(cmdlists);
	PrintCmdLists(cmdlists);

//	sleep(11);
//	LOG(INFO)<<"main thread clear all\n";
//	ddr.ClearExecutingInviteCmdList();

	pthread_join(t0, NULL);
//	pthread_join(t1, NULL);

	ddr.Stop();
	ddr.Uninit();
}


int main(int argc, char** argv)
{
    string log_config_filename="log.conf";
    if(argc<2)
    {
        printf("usage: %s configFileName\n", argv[0]);
        printf("default: %s\n", log_config_filename.c_str());
    }
    else
    {
        log_config_filename=argv[1];
        printf("log config filename %s\n", log_config_filename.c_str());
    }

    OWLog::config(log_config_filename);

//	TestTransactionSetAdd();
	printf("********************\n");
//    TestDownDataRestorerDevice();
	printf("********************\n");
    TestDownDataRestorerExecutingInviteCmd();
    
    return 0;
}
