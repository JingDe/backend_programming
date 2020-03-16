#include"redisclient.h"
#include"down_data_restorer.h"
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

using namespace GBGateway;

DownDataRestorer* g_ddr=NULL;

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


void print(const list<Device>& devices)
{
	LOG(INFO)<<"print devices: ["<<devices.size()<<"]";
	for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); ++it)
	{
		LOG(INFO)<<"deviceId="<<it->deviceId<<",";
	}
	LOG(INFO)<<"";
}

void ShowDevices(DownDataRestorer& ddr)
{
	list<Device> devices;
	ddr.LoadDeviceList(devices);
	print(devices);
}

void LoopProcessDevice(DownDataRestorer& ddr)
{
	int device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"devices size: "<<device_count;

	assert(device_count==0);

	Device device1;
	device1.deviceId="device_id_1";
	ddr.InsertDeviceList(device1);

	sleep(3);
	
	device_count=ddr.GetDeviceCount();
	LOG(INFO)<<"after insert, devices size: "<<device_count;

	LOG(INFO)<<"after insert, before search, to check redis-cli";


	Device device;
	if(ddr.SelectDeviceList("device_id_1", device)==DDR_OK)
	{
		LOG(INFO)<<"search device_id_1 success";
	}
	else
	{
		LOG(INFO)<<"search device_id_1 failed";
	}

	Device device2;
	device2.deviceId="device_id_2";
	ddr.InsertDeviceList(device2);

	Device device3;
	device3.deviceId="device_id_3";
	ddr.InsertDeviceList(device3);

	Device device4;
	device4.deviceId="device_id_4";
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

void TestMallocDevice()
{
	Device d;
	std::cout<<sizeof(d)<<std::endl;

	Device d2;
	d2.deviceStateChangeTime->GetCurrentTime();
	std::cout<<sizeof(d2)<<std::endl;
}

int main(int argc, char** argv)
{
	signal(SIGSEGV, CallbackSignal);

	Glogger glog;

	TestMallocDevice();


	TestDDRInStandaloneMode();
	
    return 0;
}
