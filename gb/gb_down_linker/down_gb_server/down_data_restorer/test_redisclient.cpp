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

const std::string gbdownlinker_device_id="070aa23c-7167-4f55-8cde-8ab7e870f1d6";

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
	ddr.LoadDeviceList(gbdownlinker_device_id, &devices);
	print(devices);
}

void LoopProcessDevice(CDownDataRestorerRedis& ddr)
{
	int device_count=ddr.GetDeviceCount(gbdownlinker_device_id);
	LOG(INFO)<<"devices size: "<<device_count;

	assert(device_count==0);

	Device device1;
	device1.deviceId="device_id_1";
	ddr.InsertDeviceList(gbdownlinker_device_id, &device1);

	sleep(3);
	
	device_count=ddr.GetDeviceCount(gbdownlinker_device_id);
	LOG(INFO)<<"after insert, devices size: "<<device_count;

	LOG(INFO)<<"after insert, before search, to check redis-cli";

	Device device2;
	device2.deviceId="device_id_2";
	ddr.InsertDeviceList(gbdownlinker_device_id, &device2);

	Device device3;
	device3.deviceId="device_id_3";
	ddr.InsertDeviceList(gbdownlinker_device_id, &device3);

	Device device4;
	device4.deviceId="device_id_4";
	ddr.InsertDeviceList(gbdownlinker_device_id, &device4);

	sleep(3);
	ShowDevices(ddr);

	LOG(INFO)<<"to delete device1";
	ddr.DeleteDeviceList(gbdownlinker_device_id, device1.deviceId);

	sleep(3);
	ShowDevices(ddr);

	LOG(INFO)<<"to clear";
	ddr.DeleteDeviceList(gbdownlinker_device_id);
	sleep(3);
	
	device_count=ddr.GetDeviceCount(gbdownlinker_device_id);
	LOG(INFO)<<"after clear, devices size: "<<device_count;
	ShowDevices(ddr);

	sleep(3);
	LOG(INFO)<<"after update";
	ShowDevices(ddr);

	sleep(3);
	LOG(INFO)<<"to clear";
	ddr.DeleteDeviceList(gbdownlinker_device_id);
	sleep(3);
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

//	while(true)
	{
		LoopProcessDevice(ddr);

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

