#include"downdatastorer.h"
#include"countdownlatch.h"
#include"device.h"
#include"channel.h"
#include"executinginvitecmd.h"
#include"glog/logging.h"

DownDataRestorer::DownDataRestorer()
	:redis_client_(NULL),
	inited_(false),
	started_(false),
	data_restorer_threads_num_(0),
	data_restorer_threads_(),
	data_restorer_threads_exit_latch(NULL)
{
	google::InitGoogleLogging("downdatarestorer");
	FLAGS_logtostderr=false;
	FLAGS_log_dir="./downdatarestorerlogdir";
	FLAGS_alsologtostderr=true;
	FLAGS_colorlogtostderr=true;
	
	LOG(INFO)<<"DownDataRestorer constructed ok";
}

DownDataRestorer::~DownDataRestorer()
{
	LOG(INFO)<<"DownDataRestorer descontruction";

	if(started_)
		Stop();
	if(inited_)
		Uninit();
	google::ShutdownGoogleLogging();	
}

int DownDataRestorer::Init(string redis_server_ip, uint16_t redis_server_port, int connection_num)
{
	if(inited_)
	{
		LOG(WARNING)<<"init called repeatedly";
		return DDR_FAIL;
	}
	LOG(INFO)<<"DownDataRestorer init, redis ip "<<redis_server_ip<<", redis port"<<redis_server_port<<", connection num "<<connection_num;

	RedisServerInfo redis_server(redis_server_ip, redis_server_port);
	REDIS_SERVER_LIST redis_server_list({redis_server});
	data_restorer_threads_num_=connection_num;
	uint32_t keepalive_time_secs=86400;

	redis_client_=new RedisClient();
	if(redis_client_==NULL)
	{
		LOG(ERROR)<<"create RedisClient failed";
		return DDR_FAIL;
	}
	
	if(redis_client_.init(redis_server_list, connection_num, keepalive_time_secs)==false)
	{
		LOG(ERROR)<<"init RedisClient failed";
		return -1;
	}
	inited_=true;
	return DDR_OK;
}

int DownDataRestorer::Uninit()
{
	if(inited_  &&  redis_client_)
	{
		redis_client_.freeRedisClient();
		redis_client_=NULL;
	}
	return DDR_OK;
}

int DownDataRestorer::Start()
{
	if(!inited_)
	{
		LOG(WARNING)<<"please init DownDataRestorer before";
		return DDR_FAIL;
	}

	if(started_)
	{
		LOG(WARNING)<<"start called repeatedly";
		return DDR_FAIL;
	}

	data_restorer_threads_.reserve(data_restorer_threads_num_);
	int ret=0;
	pthread_t thread_id;
	for(int i=0; i<data_restorer_threads_num_; i++)
	{		
		ret=pthread_create(&thread_id, NULL, DataRestorerThreadFuncWrapper, this);
		if(ret!=0)
		{
			LOG(WARNING)<<"pthread_create failed";
		}
		else
		{
			data_restorer_threads_.push_back(thread_id);
		}
	}
	if(data_restorer_threads_.empty())
	{
		LOG(ERROR)<<"start redis threads failed";
		return DDR_FAIL;
	}
	data_restorer_threads_.shrink_to_fit();
	data_restorer_threads_exit_latch=new CountDownLatch(data_restorer_threads_.size());
	if(data_restorer_threads_exit_latch.IsValid()==false)
	{
		LOG(ERROR)<<"init CountDownLatch failed";
		return DDR_FAIL;
	}
	started_=true;
	return DDR_OK;
}

int DownDataRestorer::Stop()
{
	if(!started_)
	{
		LOG(WARNING)<<"not started yet";
		retun DDR_FAIL;
	}

	force_thread_exit=true;
	data_restorer_threads_exit_latch.wait();

	started_=false;
	return DDR_OK;
}

void* DownDataRestorer::DataRestorerThreadFuncWrapper(void* arg)
{
	DownDataRestorer* down_data_restorer=(DownDataRestorer*)arg;
	down_data_restorer->DataRestorerThreadFunc();
	return 0;
}

void DownDataRestorer::DataRestorerThreadFunc()
{
	
}

int DownDataRestorer::GetStat()
{
	return DDR_OK;
}

int DownDataRestorer::LoadDeviceList(list<Device>& devices)
{
	
}