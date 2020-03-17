#include"redisclient.h"
#include"down_data_restorer_def.h"
#include"down_data_restorer.h"
#include"down_data_restorer_redis.h"
#include"device.h"
#include"glog/logging.h"
#include"glog/raw_logging.h"
#include<signal.h>
#include<execinfo.h>
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

//const std::string g_gbdownlinker_device_id="070aa23c-7167-4f55-8cde-8ab7e870f1d6";
const std::string g_gbdownlinker_device_id="gbdownlinker_device_id";

using namespace GBGateway;


#define ADDR_MAX_NUM 100

void CallbackSignal (int iSignalNo) {
    printf ("CALLBACK: SIGNAL: %d\n", iSignalNo);
    void *pBuf[ADDR_MAX_NUM] = {0};
    int iAddrNum = backtrace(pBuf, ADDR_MAX_NUM);
    printf("BACKTRACE: NUMBER OF ADDRESSES IS:%d\n\n", iAddrNum);
    char ** strSymbols = backtrace_symbols(pBuf, iAddrNum);
    if (strSymbols == NULL) {
        printf("BACKTRACE: CANNOT GET BACKTRACE SYMBOLS\n");
        return;
    }
    int ii = 0;
    for (ii = 0; ii < iAddrNum; ii++) {
        printf("%03d %s\n", iAddrNum-ii, strSymbols[ii]);
    }
    printf("\n");
    free(strSymbols);
    strSymbols = NULL;
    exit(1); // QUIT PROCESS. IF NOT, MAYBE ENDLESS LOOP.
}


void print(const std::list<DevicePtr>& devices)
{
	LOG(INFO)<<"print devices: ["<<devices.size()<<"]";
	for(list<DevicePtr>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		LOG(INFO)<<"deviceId="<<(*it)->deviceId<<",";
	}
	LOG(INFO)<<"";
}

void ShowDevices(CDownDataRestorerRedis& ddr)
{
	list<DevicePtr> devices;
	ddr.LoadDeviceList(g_gbdownlinker_device_id, &devices);
	print(devices);
}


void LoopProcessDevice(CDownDataRestorerRedis& ddr)
{
	int device_count = 0;
	std::stringstream log_msg;

	LOG_WRITE_INFO("test load device");
	// load接口
	std::list<DevicePtr> device_list;
	ddr.LoadDeviceList(g_gbdownlinker_device_id, &device_list);
	//assert(device_list.empty());
	log_msg<<"load device, size is "<<device_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("test insert device");
	// insert接口
	Device device1;
	device1.deviceId="device_id_1";
	ddr.InsertDeviceList(g_gbdownlinker_device_id, &device1);
	sleep(7);
	device_list.clear();
	ddr.LoadDeviceList(g_gbdownlinker_device_id, &device_list);
//	assert(device_list.size()==1);
	log_msg.str("");
	log_msg<<"device size is "<<device_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getchar();

	LOG_WRITE_INFO("test update devie");	
	// update接口
	ddr.UpdateDeviceList(g_gbdownlinker_device_id, &device1);
	sleep(7);
	device_list.clear();
	ddr.LoadDeviceList(g_gbdownlinker_device_id, &device_list);
	//assert(device_list.size() == 1);
	log_msg.str("");
    log_msg<<"device size is "<<device_list.size();
    LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("insert 3 device");
	// insert接口
	Device device2;
	device2.deviceId="device_id_2";
	ddr.InsertDeviceList(g_gbdownlinker_device_id, &device2);
	Device device3;
	device3.deviceId="device_id_3";
	ddr.InsertDeviceList(g_gbdownlinker_device_id, &device3);
	Device device4;
	device4.deviceId="device_id_4";
	ddr.InsertDeviceList(g_gbdownlinker_device_id, &device4);
	sleep(7);
	device_list.clear();
	ddr.LoadDeviceList(g_gbdownlinker_device_id, &device_list);
//	assert(device_list.size() == 4);
	log_msg.str("");
    log_msg<<"device size is "<<device_list.size();
    LOG_WRITE_INFO(log_msg.str());
	getchar();

	LOG_WRITE_INFO("test delete device");
	// delete接口
	ddr.DeleteDeviceList(g_gbdownlinker_device_id, device1.deviceId);
	sleep(7);
	device_list.clear();
	ddr.LoadDeviceList(g_gbdownlinker_device_id, &device_list);
	//assert(device_list.size() == 3);
	log_msg.str("");
    log_msg<<"device size is "<<device_list.size();
    LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("test get device count");
	// get count 接口
	device_count=ddr.GetDeviceCount(g_gbdownlinker_device_id);
	//assert(device_count == 3);
	log_msg.str("");
    log_msg<<"device count "<<device_count;
    LOG_WRITE_INFO(log_msg.str());
	getchar();

	LOG_WRITE_INFO("test delete all device");
	// delete/clear 接口
	ddr.DeleteDeviceList(g_gbdownlinker_device_id);
	sleep(5);
	device_count = ddr.GetDeviceCount(g_gbdownlinker_device_id);
	//assert(device_count == 0);
	log_msg.str("");
    log_msg<<"device count "<<device_count;
    LOG_WRITE_INFO(log_msg.str());
	getchar();
}

void LoopProcessChannel(CDownDataRestorerRedis& ddr)
{
	int channel_count = 0;
	std::stringstream log_msg;

	LOG_WRITE_INFO("test load channel");
	// load接口
	std::list<ChannelPtr> channel_list;
	ddr.LoadChannelList(g_gbdownlinker_device_id, &channel_list);
	log_msg<<"load channel, size is "<<channel_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("insert some channel");
	// insert接口
	Channel channel11;
	channel11.deviceId="device_id1";
    channel11.channelDeviceId="channel_device_id_1";
	ddr.InsertChannelList(g_gbdownlinker_device_id, &channel11);
	Channel channel12;
	channel12.deviceId="device_id1";
    channel12.channelDeviceId="channel_device_id_2";
	ddr.InsertChannelList(g_gbdownlinker_device_id, &channel12);
	Channel channel13;
	channel13.deviceId="device_id1";
    channel13.channelDeviceId="channel_device_id_3";
	ddr.InsertChannelList(g_gbdownlinker_device_id, &channel13);
	
	Channel channel24;
	channel24.deviceId="device_id2";
    channel24.channelDeviceId="channel_device_id_4";
	ddr.InsertChannelList(g_gbdownlinker_device_id, &channel24);
	Channel channel25;
	channel25.deviceId="device_id2";
    channel25.channelDeviceId="channel_device_id_5";
	ddr.InsertChannelList(g_gbdownlinker_device_id, &channel25);
	sleep(7);
	channel_list.clear();
	ddr.LoadChannelList(g_gbdownlinker_device_id, &channel_list);
	log_msg.str("");
    log_msg<<"channel size is "<<channel_list.size();
    LOG_WRITE_INFO(log_msg.str());
	getchar();

	LOG_WRITE_INFO("test delete by device_id and channel_device_id");
	// delete接口
	ddr.DeleteChannelList(g_gbdownlinker_device_id, channel11.deviceId, channel11.channelDeviceId);
	sleep(7);
	channel_list.clear();
	ddr.LoadChannelList(g_gbdownlinker_device_id, &channel_list);
	log_msg.str("");
    log_msg<<"channel size is "<<channel_list.size();
    LOG_WRITE_INFO(log_msg.str());
	getc(stdin);
	
	LOG_WRITE_INFO("test delete by device_id");
	// delete接口
	ddr.DeleteChannelList(g_gbdownlinker_device_id, channel24.deviceId);
	sleep(7);
	channel_list.clear();
	ddr.LoadChannelList(g_gbdownlinker_device_id, &channel_list);
	log_msg.str("");
    log_msg<<"channel size is "<<channel_list.size();
    LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("test get channel count");
	// get count 接口
	channel_count=ddr.GetChannelCount(g_gbdownlinker_device_id);
	//assert(channel_count == 3);
	log_msg.str("");
    log_msg<<"channel count "<<channel_count;
    LOG_WRITE_INFO(log_msg.str());
	getchar();

	LOG_WRITE_INFO("test delete all channel");
	// delete/clear 接口
	ddr.DeleteChannelList(g_gbdownlinker_device_id);
	sleep(5);
	channel_count = ddr.GetChannelCount(g_gbdownlinker_device_id);
	log_msg.str("");
    log_msg<<"channel count "<<channel_count;
    LOG_WRITE_INFO(log_msg.str());
	getchar();
}


void LoopProcessExecutingInviteCmd(CDownDataRestorerRedis& ddr)
{
	int invite_count = 0;
	int worker_thread_idx=0;
	std::list<ExecutingInviteCmdPtr> invite_list;
	
	std::stringstream log_msg;

	LOG_WRITE_INFO("test load invite");
	worker_thread_idx=0;
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg<<"load invite of thread 0, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);	
	worker_thread_idx=1;
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg.str("");
	log_msg<<"load invite of thread 1, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("insert some invite");
	ExecutingInviteCmd invite01;
	invite01.cmdSenderId="cmd_sender_id_1";
	invite01.deviceId="device_id1";
    invite01.cmdSeq=1;
	ddr.InsertExecutingInviteCmdList(g_gbdownlinker_device_id, 0, &invite01);
	ExecutingInviteCmd invite02;
	invite02.cmdSenderId="cmd_sender_id_2";
	invite02.deviceId="device_id2";
    invite02.cmdSeq=2;
	ddr.InsertExecutingInviteCmdList(g_gbdownlinker_device_id, 0, &invite02);
	ExecutingInviteCmd invite03;
	invite03.cmdSenderId="cmd_sender_id_3";
	invite03.deviceId="device_id3";
    invite03.cmdSeq=3;
	ddr.InsertExecutingInviteCmdList(g_gbdownlinker_device_id, 0, &invite03);	
	
	ExecutingInviteCmd invite14;
	invite14.cmdSenderId="cmd_sender_id_4";
	invite14.deviceId="device_id4";
    invite14.cmdSeq=4;
	ddr.InsertExecutingInviteCmdList(g_gbdownlinker_device_id, 1, &invite14);
	ExecutingInviteCmd invite15;
	invite15.cmdSenderId="cmd_sender_id_5";
	invite15.deviceId="device_id5";
    invite15.cmdSeq=5;
	ddr.InsertExecutingInviteCmdList(g_gbdownlinker_device_id, 1, &invite15);
	sleep(7);
	
	worker_thread_idx=0;
	invite_list.clear();
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg.str("");
	log_msg<<"load invite of thread 0, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);	
	worker_thread_idx=1;
	invite_list.clear();
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg.str("");
	log_msg<<"load invite of thread 1, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("test get invite count");
	worker_thread_idx=0;
	invite_count=ddr.GetExecutingInviteCmdCount(g_gbdownlinker_device_id, worker_thread_idx);
	log_msg.str("");
    log_msg<<"invite count of worker 0 is "<<invite_count;
    LOG_WRITE_INFO(log_msg.str());
	getchar();
	worker_thread_idx=1;
	invite_count=ddr.GetExecutingInviteCmdCount(g_gbdownlinker_device_id, worker_thread_idx);
	log_msg.str("");
    log_msg<<"invite count of worker 1 is "<<invite_count;
    LOG_WRITE_INFO(log_msg.str());
	getchar();

	LOG_WRITE_INFO("test delete by 3 arg");
	ddr.DeleteExecutingInviteCmdList(g_gbdownlinker_device_id, 0, invite01.cmdSenderId, invite01.deviceId, invite01.cmdSeq);
	sleep(7);
	invite_list.clear();
	worker_thread_idx=0;
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg.str("");
	log_msg<<"load invite of thread 0, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);	
	worker_thread_idx=1;
	invite_list.clear();
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg.str("");
	log_msg<<"load invite of thread 1, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);
	
	LOG_WRITE_INFO("test delete by worker_thread_idx");
	ddr.DeleteExecutingInviteCmdList(g_gbdownlinker_device_id, 1);
	sleep(7);	
	worker_thread_idx=0;
	invite_list.clear();
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg.str("");
	log_msg<<"load invite of thread 0, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);	
	worker_thread_idx=1;
	invite_list.clear();
	ddr.LoadExecutingInviteCmdList(g_gbdownlinker_device_id, worker_thread_idx, &invite_list);
	log_msg.str("");
	log_msg<<"load invite of thread 1, size is "<<invite_list.size();
	LOG_WRITE_INFO(log_msg.str());
	getc(stdin);

	LOG_WRITE_INFO("delete all invite");
	ddr.DeleteExecutingInviteCmdList(g_gbdownlinker_device_id, 0);
	getchar();
}


void TestDDRInSentinelMode()
{
	RestorerParam::RedisConfig redis_config;
	redis_config.masterName = "mymaster";

	RestorerParam::RedisConfig::UrlPtr url1 = std::make_unique<RestorerParam::RedisConfig::Url>();
	url1->ip = "192.168.12.59";
	url1->port = 26379;
	redis_config.urlList.push_back(std::move(url1));

	RestorerParam::RedisConfig::UrlPtr url2 = std::make_unique<RestorerParam::RedisConfig::Url>();
	url2->ip = "192.168.12.59";
	url2->port = 26380;
	redis_config.urlList.push_back(std::move(url2));

	RestorerParam::RedisConfig::UrlPtr url3 = std::make_unique<RestorerParam::RedisConfig::Url>();
	url3->ip = "192.168.12.59";
	url3->port = 26381;
	redis_config.urlList.push_back(std::move(url3));

	RestorerParamPtr param = std::make_unique<RestorerParam>();
	param->type = RestorerParam::Type::Redis;
	param->config = std::move(redis_config);


	CDownDataRestorerRedis ddr;

	if(ddr.Init(param)==RESTORER_FAIL)
		return;
	if(ddr.Start()==RESTORER_FAIL)
		return;

	ddr.PrepareLoadData(g_gbdownlinker_device_id, 3);

//	while(true)
	{
	//	LoopProcessDevice(ddr);
//		LoopProcessChannel(ddr);
//		LoopProcessExecutingInviteCmd(ddr);

//		LOG(WARNING)<<"wait to loop";
//		sleep(10);
	}

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
    //  google::InitGoogleLogging("main");
    //  FLAGS_logtostderr=false;
    //  FLAGS_log_dir="./downdatarestorerlogdir";
    //  FLAGS_alsologtostderr=true;
    //  FLAGS_colorlogtostderr=true;

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


int main(int argc, char** argv)
{
	signal(SIGSEGV, CallbackSignal);

	Glogger glog;



	TestDDRInSentinelMode();
	
    return 0;
}

