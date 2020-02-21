#include"redisclient.h"
#include"downdatarestorer.h"
#include"device.h"
#include"glog/logging.h"
#include"glog/raw_logging.h"
//#include"owlog.h"
#include<string>
#include<list>
#include<iostream>
#include<cstdio>
#include<unistd.h>
#include<fstream>

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
//	uint32_t keepalive_time=60;
	if(client.init(redis_ip, redis_port, conn_num)==false)
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

bool TestDelTransactionInCluster()
{
	RedisClient client;
    LOG(INFO)<<"construct RedisClient ok";

	RedisServerInfo server0("192.168.12.59", 7000);
	RedisServerInfo server1("192.168.12.59", 7001);
	RedisServerInfo server2("192.168.12.59", 7002);
	RedisServerInfo server3("192.168.12.59", 7003);
	RedisServerInfo server4("192.168.12.59", 7004);
	RedisServerInfo server5("192.168.12.59", 7005);
	REDIS_SERVER_LIST servers({server0, server1, server2, server3, server4, server5});
	uint32_t conn_num=8;
//	uint32_t keepalive_time=-1;
	if(client.init(servers, conn_num/*, keepalive_time*/)==false)
	{
		LOG(ERROR)<<"client init failed";
		return false;
	}

	RedisConnection *con=NULL;
	do{
	    if(!client.PrepareTransaction(&con))
	    	break;
	    	
	    if(!client.StartTransaction(con))
	    	break;
	    if(!client.Srem(con, "testset", "member1"))
	    	break;
	    if(!client.Del(con, "member1"))
	    	break;
	    if(!client.ExecTransaction(con))
	    	break;
	    	
	    client.FinishTransaction(&con);
	    LOG(INFO)<<"transaction success";
	    		    
    }while(0);

    if(con)
    {
    	LOG(ERROR)<<"transaction failed";
    	client.FinishTransaction(&con);
    }

    return true;
}


bool TestTransactionSetDel()
{
    RedisClient client;
    printf("construct RedisClient ok\n");

//	RedisServerInfo server(redis_ip, redis_port);
//	REDIS_SERVER_LIST servers({server});
	uint32_t conn_num=10;
//	uint32_t keepalive_time=60;
	if(client.init(redis_ip, redis_port, conn_num)==false)
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
	LOG(INFO)<<"print devices: ["<<devices.size()<<"]";
	for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		LOG(INFO)<<"deviceId="<<it->GetDeviceId()<<",";
	}
	LOG(INFO)<<"";
}

void ShowDevices(DownDataRestorer& ddr)
{
	list<Device> devices;
	ddr.LoadDeviceList(devices);
	print(devices);
}

void TestDownDataRestorerDevice()
{
	DownDataRestorer ddr;
#if 0
	ddr.Init(redis_ip, redis_port, 8);
#else
	RedisServerInfo server0("192.168.12.59", 7000);
	RedisServerInfo server1("192.168.12.59", 7001);
	RedisServerInfo server2("192.168.12.59", 7002);
	RedisServerInfo server3("192.168.12.59", 7003);
	RedisServerInfo server4("192.168.12.59", 7004);
	RedisServerInfo server5("192.168.12.59", 7005);
	REDIS_SERVER_LIST servers({server0, server1, server2, server3, server4, server5});
	ddr.Init(servers, 8);
#endif
	if(ddr.Inited()==false)
		return;
		
	ddr.Start();

	LOG(INFO)<<"TestDownDataRestorerDevice start";
	
	int device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"devices size: "<<device_count;

	Device device1("device_id_1");
	ddr.InsertDeviceList(device1);

	sleep(3);
	
	device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"after insert, devices size: "<<device_count;

	LOG(INFO)<<"after insert, before search, to check redis-cli";
//	getchar();
	//getc(stdin);

	Device device;
	if(ddr.SelectDeviceList("device_id_1", device)==DDR_OK)
	{
		LOG(INFO)<<"search device_id_1 success";
	}
	else
	{
		LOG(INFO)<<"search device_id_1 failed";
	}
//	getchar();

	Device device2("device_id_2");
	ddr.InsertDeviceList(device2);

	Device device3("device_id_3");
	ddr.InsertDeviceList(device3);

	Device device4("device_id_4");
	ddr.InsertDeviceList(device4);
	
	ShowDevices(ddr);

	LOG(INFO)<<"to delete device1";
	ddr.DeleteDeviceList(device1);

	sleep(3);
	ShowDevices(ddr);

	LOG(INFO)<<"to clear";
	ddr.ClearDeviceList();
	sleep(3);
	
	device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"after clear, devices size: "<<device_count;
	ShowDevices(ddr);

	LOG(INFO)<<"to update";
	list<Device> devices({device1, device3});
	ddr.UpdateDeviceList(devices);

	sleep(3);
	LOG(INFO)<<"after update";
	ShowDevices(ddr);

	ddr.Stop();
	ddr.Uninit();
}

void print(const list<Channel>& channels)
{
	LOG(INFO)<<"print channels: ["<<channels.size()<<"]";
	for(list<Channel>::const_iterator it=channels.begin(); it!=channels.end(); ++it)
	{
		LOG(INFO)<<"channelId="<<it->GetChannelId()<<",";
	}
	LOG(INFO)<<"";
}

void ShowChannels(DownDataRestorer& ddr)
{
	list<Channel> channels;
	ddr.LoadChannelList(channels);
	print(channels);
}

void TestDownDataRestorerChannel()
{
	DownDataRestorer ddr;
#if 0
	ddr.Init(redis_ip, redis_port, 8);
#else
	RedisServerInfo server0("192.168.12.59", 7000);
	RedisServerInfo server1("192.168.12.59", 7001);
	RedisServerInfo server2("192.168.12.59", 7002);
	RedisServerInfo server3("192.168.12.59", 7003);
	RedisServerInfo server4("192.168.12.59", 7004);
	RedisServerInfo server5("192.168.12.59", 7005);
	REDIS_SERVER_LIST servers({server0, server1, server2, server3, server4, server5});
	ddr.Init(servers, 8);
#endif
	if(ddr.Inited()==false)
		return;
		
	ddr.Start();

	LOG(INFO)<<"TestDownDataRestorerChannel start";
	
	int channel_count=ddr.GetChannelCount();
	LOG(INFO)<<"channels size: "<<channel_count;

	Channel channel1("channel_id_1", "channel_id_1");
	ddr.InsertChannelList(channel1);

	sleep(3);
	
	channel_count=ddr.GetChannelCount();
	LOG(INFO)<<"after insert, channels size: "<<channel_count;

	LOG(INFO)<<"after insert, before search, to check redis-cli";
//	getchar();
	//getc(stdin);

	Channel channel;
	if(ddr.SelectChannelList("channel_id_1channel_id_1", channel)==DDR_OK)
	{
		LOG(INFO)<<"search channel_id_1 success";
	}
	else
	{
		LOG(INFO)<<"search channel_id_1 failed";
	}
//	getchar();

	Channel channel2("channel_id_2", "channel_id_2");
	ddr.InsertChannelList(channel2);

	Channel channel3("channel_id_3", "channel_id_3");
	ddr.InsertChannelList(channel3);

	Channel channel4("channel_id_4", "channel_id_4");
	ddr.InsertChannelList(channel4);
	
	ShowChannels(ddr);

	LOG(INFO)<<"to delete channel1";
	ddr.DeleteChannelList(channel1);

	sleep(3);
	ShowChannels(ddr);

	LOG(INFO)<<"to clear";
	ddr.ClearChannelList();
	sleep(3);
	
	channel_count=ddr.GetChannelCount();
	LOG(INFO)<<"after clear, channels size: "<<channel_count;
	ShowChannels(ddr);

	LOG(INFO)<<"to update";
	list<Channel> channels({channel1, channel3});
	ddr.UpdateChannelList(channels);

	sleep(3);
	LOG(INFO)<<"after update";
	ShowChannels(ddr);

	ddr.Stop();
	ddr.Uninit();
}


void PrintCmdList(const ExecutingInviteCmdList& cmdlist, int set_no)
{
	LOG(INFO)<<"start show the "<<set_no<<" cmd set, ["<<cmdlist.size()<<"]\n";
	for(ExecutingInviteCmdList::const_iterator it=cmdlist.begin(); it!=cmdlist.end(); ++it)
	{
		LOG(INFO)<<it->GetId().c_str()<<"\t";
	}
}

void PrintCmdLists(const vector<ExecutingInviteCmdList>& cmdlists)
{
//	printf("*** start show cmd lists: [%d]\n", (int)cmdlists.size());
	LOG(INFO)<<"*** start show cmd lists: ["<<cmdlists.size()<<"]\n";
	for(size_t i=0; i<cmdlists.size(); ++i)
	{
		PrintCmdList(cmdlists[i], i+1);
	}
//	printf("*** end\n");
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
	int thread_no=*(reinterpret_cast<int*>(arg));
	LOG(INFO)<<"worker thread %d start"<<thread_no;

	ShowCmdList(thread_no);

	ExecutingInviteCmd cmd1(1);
	g_ddr->InsertExecutingInviteCmdList(thread_no, cmd1);

	ExecutingInviteCmd cmd2(2);
	g_ddr->InsertExecutingInviteCmdList(thread_no, cmd2);

	sleep(3);
	LOG(INFO)<<"after insert";
	ShowCmdList(thread_no);
	
//	getchar();

	ExecutingInviteCmd cmd;
	if(g_ddr->SelectExecutingInviteCmdList(thread_no, "1", cmd)==DDR_OK)
	{
		LOG(INFO)<<"select ok: "<<cmd.GetId();
	}
	else
	{
		LOG(INFO)<<"select failed";
	}

//	getchar();
	
	g_ddr->DeleteExecutingInviteCmdList(thread_no, cmd1);

	sleep(3);
	LOG(INFO)<<"after delete";
	ShowCmdList(thread_no);

//	getchar();

	ExecutingInviteCmdList cmd_list({cmd1});
	g_ddr->UpdateExecutingInviteCmdList(thread_no, cmd_list);

	sleep(3);
	LOG(INFO)<<"after update";
	ShowCmdList(thread_no);

	LOG(INFO)<<"before clear";
//	getchar();

	g_ddr->ClearExecutingInviteCmdList(thread_no);

	sleep(3);
	LOG(INFO)<<"after clear";
	ShowCmdList(thread_no);

	LOG(INFO)<<"worker thread "<<thread_no<<" exit";	

	return 0;
}

void* WorkerThread1Func(void* arg)
{
	

	return 0;
}


void TestDownDataRestorerExecutingInviteCmd()
{
	DownDataRestorer ddr;
	
#if 0
	ddr.Init(redis_ip, redis_port, 8);
#else
	RedisServerInfo server0("192.168.12.59", 7000);
	RedisServerInfo server1("192.168.12.59", 7001);
	RedisServerInfo server2("192.168.12.59", 7002);
	RedisServerInfo server3("192.168.12.59", 7003);
	RedisServerInfo server4("192.168.12.59", 7004);
	RedisServerInfo server5("192.168.12.59", 7005);
	REDIS_SERVER_LIST servers({server0, server1, server2, server3, server4, server5});
	ddr.Init(servers, 8);
#endif

	if(ddr.Inited()==false)
		return;

	ddr.Start();
	g_ddr=&ddr;

	int worker_thread_no_0=0;
	LOG(INFO)<<"in main, "<<worker_thread_no_0<<", "<<&worker_thread_no_0;
	pthread_t t0=StartWorkerThread(worker_thread_no_0, WorkerThread0Func);
	int worker_thread_no_1=1;
//	pthread_t t1=StartWorkerThread(worker_thread_no_1, WorkerThread1Func);


	sleep(3);
	{
		vector<ExecutingInviteCmdList> cmdlists;
		ddr.LoadExecutingInviteCmdList(cmdlists);
		PrintCmdLists(cmdlists);
	}

	sleep(10);
	LOG(INFO)<<"main thread CLEAR all";
	ddr.ClearExecutingInviteCmdList();

	{
		LOG(INFO)<<"main thread UPDATE now";
		ExecutingInviteCmd cmd11(11);
		ExecutingInviteCmd cmd12(12);
		ExecutingInviteCmdList cmd_list1({cmd11, cmd12});
		ExecutingInviteCmd cmd21(21);
		ExecutingInviteCmd cmd22(22);
		ExecutingInviteCmdList cmd_list2({cmd21, cmd22});
		vector<ExecutingInviteCmdList> cmdlists({cmd_list1, cmd_list2});
		ddr.UpdateExecutingInviteCmdList(cmdlists);
	}

	sleep(3);

	{
	vector<ExecutingInviteCmdList> cmdlists;
	ddr.LoadExecutingInviteCmdList(cmdlists);
	PrintCmdLists(cmdlists);
	}

	pthread_join(t0, NULL);
//	pthread_join(t1, NULL);

	ddr.Stop();
	ddr.Uninit();
}


//将信息输出到单独的文件和 LOG(ERROR)
void SignalHandle(const char* data, int size)
{
    std::ofstream fs("glog_dump.log", std::ios::app);
    std::string str = std::string(data,size);
    fs<<str;
    fs.close();
    LOG(ERROR)<<str;
}

class Glogger{
public:
	Glogger()
	{
	//	google::InitGoogleLogging("main");
	//	FLAGS_logtostderr=false;
	//	FLAGS_log_dir="./downdatarestorerlogdir";
	//	FLAGS_alsologtostderr=true;
	//	FLAGS_colorlogtostderr=true;

		google::InitGoogleLogging("test_ddr");
		FLAGS_log_dir="./downdatarestorerlogdir";
	    FLAGS_stderrthreshold=google::INFO;
	    FLAGS_colorlogtostderr=true;
	    google::InstallFailureWriter(&SignalHandle);
	}

	~Glogger()
	{
		google::ShutdownGoogleLogging();
	}
};

void TestSentinelSlaves()
{
	// "sentienl slaves"
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 2))
		return;

	client.DoTestOfSentinelSlavesCommand();

	return ;
}

void TestParseEnhanceWithSmembers()
{
	// "sentienl slaves"
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 2))
		return;

	string setkey="testparseset";
	list<string> members;
	client.DoSmembersWithParseEnhance(setkey, members);

	return ;
}

void TestParseEnhanceWithSadd()
{
	// "sentienl slaves"
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 2))
		return;

	string setkey="testparseset";
	string setMember="testmember";
	client.DoSaddWithParseEnhance(setkey, setMember);

	return ;
}

void TestParseEnhanceWithGet()
{
	// "sentienl slaves"
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 2))
		return;

	string strKey="norah";
	string value;
	client.DoGetWithParseEnhance(strKey, value);
	LOG(INFO)<<"get value "<<value;

	return ;
}


void TestParseEnhanceWithSet()
{
	// "sentienl slaves"
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 2))
		return;

	string strKey="name";
	string value="wangjing";
	client.DoSetWithParseEnhance(strKey, value);

	return ;
}

void TestParseEnhanceWithWrongCmd()
{
	// "sentienl slaves"
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 2))
		return;

	
	client.DoWrongCmdWithParseEnhance("foobar");

	return ;
}


void TestRedisSentinelInit()
{
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 4))
		return;

}

void TestRedisClusterInit()
{
	RedisClient client;

	RedisServerInfo cluster1("192.168.12.59", 7000);
	RedisServerInfo cluster2("192.168.12.59", 7001);
	RedisServerInfo cluster3("192.168.12.59", 7002);
	RedisServerInfo cluster4("192.168.12.59", 7003);
	RedisServerInfo cluster5("192.168.12.59", 7004);
	RedisServerInfo cluster6("192.168.12.59", 7005);
	REDIS_SERVER_LIST serverList({cluster1, cluster2, cluster3,cluster4,cluster5,cluster6});
	if(!client.init(serverList, 4))
		return;

}

void TestRedisSentinelSetApi()
{
	RedisClient client;

	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST serverList({sentinel1, sentinel2, sentinel3});
	string master_name="mymaster";
	if(!client.init(serverList, master_name, 4))
		return;

	client.sadd("sentinel01set", "sentinel01set_id01");
	client.setSerial("sentinel01set_id01", "sentinel01set_id01_value01");

}

void TestRedisSentinelVersionDDR()
{
	DownDataRestorer ddr;	
	string master_name="mymaster";
	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST sentinels({sentinel1, sentinel2, sentinel3});
	if(ddr.Init(sentinels, master_name, 3)==false)
		return;
	ddr.Start();

	// 1
	int device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"devices size: "<<device_count;

	// 2
	Device device1("device_id_1");
	ddr.InsertDeviceList(device1);

	sleep(3);
	
	device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"after insert, devices size: "<<device_count;

//	LOG(INFO)<<"after insert, before search, to check redis-cli";

	// 3
	Device device;
	if(ddr.SelectDeviceList("device_id_1", device)==DDR_OK)
	{
		LOG(INFO)<<"search device_id_1 success";
	}
	else
	{
		LOG(INFO)<<"search device_id_1 failed";
	}
//	getchar();

	Device device2("device_id_2");
	ddr.InsertDeviceList(device2);

	Device device3("device_id_3");
	ddr.InsertDeviceList(device3);

	Device device4("device_id_4");
	ddr.InsertDeviceList(device4);

	// 4
	sleep(3);
	ShowDevices(ddr);

	LOG(INFO)<<"to delete device1";

	// 5
	ddr.DeleteDeviceList(device1);

	sleep(3);
	ShowDevices(ddr);
	

	LOG(INFO)<<"to update";

	// 6
	list<Device> devices({device1, device3});
	ddr.UpdateDeviceList(devices);

	sleep(3);
	LOG(INFO)<<"after update";
	ShowDevices(ddr);

	LOG(INFO)<<"to clear";

	// 7
	ddr.ClearDeviceList();
	sleep(3);
	device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"after clear, devices size: "<<device_count;
	ShowDevices(ddr);
	
	ddr.Stop();
	ddr.Uninit();	
}

void TestRedisSentinelVersionDDRCmd()
{
	DownDataRestorer ddr;	
	string master_name="mymaster";
	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST sentinels({sentinel1, sentinel2, sentinel3});
	if(ddr.Init(sentinels, master_name, 3)==false)
		return;
	if(ddr.Start()==false)
		return;

	g_ddr=&ddr;

	int worker_thread_no_0=0;
	LOG(INFO)<<"in main, "<<worker_thread_no_0<<", "<<&worker_thread_no_0;
	pthread_t t0=StartWorkerThread(worker_thread_no_0, WorkerThread0Func);
	int worker_thread_no_1=1;
//	pthread_t t1=StartWorkerThread(worker_thread_no_1, WorkerThread1Func);


	sleep(3);
	{
		vector<ExecutingInviteCmdList> cmdlists;
		ddr.LoadExecutingInviteCmdList(cmdlists);
		PrintCmdLists(cmdlists);
	}

	sleep(10);
	LOG(INFO)<<"main thread CLEAR all";
	ddr.ClearExecutingInviteCmdList();

	{
		ExecutingInviteCmd cmd11(11);
		ExecutingInviteCmd cmd12(12);
		ExecutingInviteCmdList cmd_list1({cmd11, cmd12});
		ExecutingInviteCmd cmd21(21);
		ExecutingInviteCmd cmd22(22);
		ExecutingInviteCmdList cmd_list2({cmd21, cmd22});
		ExecutingInviteCmd cmd31(31);
		ExecutingInviteCmd cmd32(32);
		ExecutingInviteCmdList cmd_list3({cmd31, cmd32});
		vector<ExecutingInviteCmdList> cmdlists({cmd_list1, cmd_list2, cmd_list3});
		ddr.UpdateExecutingInviteCmdList(cmdlists);
	}

	sleep(3);

	{
	vector<ExecutingInviteCmdList> cmdlists;
	ddr.LoadExecutingInviteCmdList(cmdlists);
	PrintCmdLists(cmdlists);
	}

	pthread_join(t0, NULL);
//	pthread_join(t1, NULL);

	ddr.Stop();
	ddr.Uninit();
}


void LoopProcessDevice(DownDataRestorer& ddr)
{
	int device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"devices size: "<<device_count;

	Device device1("device_id_1");
	ddr.InsertDeviceList(device1);

	sleep(3);
	
	device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"after insert, devices size: "<<device_count;

	LOG(INFO)<<"after insert, before search, to check redis-cli";
//	getchar();
	//getc(stdin);

	Device device;
	if(ddr.SelectDeviceList("device_id_1", device)==DDR_OK)
	{
		LOG(INFO)<<"search device_id_1 success";
	}
	else
	{
		LOG(INFO)<<"search device_id_1 failed";
	}
//	getchar();

	Device device2("device_id_2");
	ddr.InsertDeviceList(device2);

	Device device3("device_id_3");
	ddr.InsertDeviceList(device3);

	Device device4("device_id_4");
	ddr.InsertDeviceList(device4);

	sleep(3);
	ShowDevices(ddr);

	LOG(INFO)<<"to delete device1";
	ddr.DeleteDeviceList(device1);

	sleep(3);
	ShowDevices(ddr);

	LOG(INFO)<<"to clear";
	ddr.ClearDeviceList();
	sleep(3);
	
	device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"after clear, devices size: "<<device_count;
	ShowDevices(ddr);

	LOG(INFO)<<"to update";
	list<Device> devices({device1, device3});
	ddr.UpdateDeviceList(devices);

	sleep(3);
	LOG(INFO)<<"after update";
	ShowDevices(ddr);

	sleep(3);
	LOG(INFO)<<"to clear";
	ddr.ClearDeviceList();
	sleep(3);
}

void TestDDRInStandaloneMode()
{
	DownDataRestorer ddr;
	if(ddr.Init(redis_ip, 6379, 4)==DDR_FAIL)
		return;
	if(ddr.Start()==DDR_FAIL)
		return;

	while(true)
	{
		LoopProcessDevice(ddr);

		LOG(WARNING)<<"wait to loop";
		sleep(10);
	}

	ddr.Stop();
	ddr.Uninit();
}


void TestDDRWhenSentinelException()
{
	DownDataRestorer ddr;	
	string master_name="mymaster";
	RedisServerInfo sentinel1("192.168.12.59", 26379);
	RedisServerInfo sentinel2("192.168.12.59", 26380);
	RedisServerInfo sentinel3("192.168.12.59", 26381);
	REDIS_SERVER_LIST sentinels({sentinel1, sentinel2, sentinel3});
	if(ddr.Init(sentinels, master_name, 3)==false)
		return;
	if(ddr.Start()==false)
		return;

	while(true)
	{
		LoopProcessDevice(ddr);

		LOG(WARNING)<<"wait to loop";
		sleep(15);
	}

	ddr.Stop();
	ddr.Uninit();
}


void TestRedisClientAuth()
{
	RedisClient client;
	client.init(redis_ip, 6379, 5, 900, 3000, "wrong");
	
}


int main(int argc, char** argv)
{
	Glogger glog;

//    string log_config_filename="log.conf";
//    if(argc<2)
//    {
//        printf("usage: %s configFileName\n", argv[0]);
//        printf("default: %s\n", log_config_filename.c_str());
//    }
//    else
//    {
//        log_config_filename=argv[1];
//        printf("log config filename %s\n", log_config_filename.c_str());
//    }

//    OWLog::config(log_config_filename);

//	TestTransactionSetAdd();
//	TestTransactionSetDel();


//    TestDownDataRestorerDevice();

//	LOG(INFO)<<"********************";
//    TestDownDataRestorerChannel();


//    TestDownDataRestorerExecutingInviteCmd();

//	TestDelTransactionInCluster();


//    TestSentinelSlaves();

//    TestRedisSentinelInit();

//    TestRedisClusterInit();

//	TestRedisSentinelSetApi();

//	TestRedisSentinelVersionDDRDevice();

//	TestRedisSentinelVersionDDRCmd();

//	TestDDRInStandaloneMode();

//	TestDDRWhenSentinelException();

//	TestRedisClientAuth();

//	TestParseEnhanceWithSmembers();

//	TestParseEnhanceWithSadd();

//	TestParseEnhanceWithGet();

//	TestParseEnhanceWithSet();

	TestParseEnhanceWithWrongCmd();
	
    return 0;
}
