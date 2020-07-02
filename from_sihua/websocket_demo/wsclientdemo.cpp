#include"wsclientdemo.cpp"

DemoWebsocketClient::DemoWebsocketClient(const std::string& server_ip, unsigned int server_port, const std::string& server_path)
	:server_ip_(server_ip),
	server_port_(server_port),
	server_path_(server_path),
	started_(false)
{}

DemoWebsocketClient::~DemoWebsocketClient()
{
	if (started_)
		Stop();
}

bool DemoWebsocketClient::Start()
{
	if (started_)
		return true;

	try {
		// 设置 WebsocketClient 的回调函数
		websocket_ = std::make_unique<WebsocketClient>(server_ip_, server_port_, server_path_, 
			std::bind(&DemoWebsocketClient::DisconnectCallback, this, std::placeholders::_1)
			std::bind(&DemoWebsocketClient::ReadCallback, this, std::placeholders::_1));
	}
	catch (...)
	{
		return false;
	}
	
	/*
		建立连接，启动线程：接收数据，并以接收到的数据为参数调用WebsocketClient的可读回调函数
	*/
	if (websocket_->open() == false)
		return false;

	started_ = true;
	return true;
}

void DemoWebsocketClient::Run()
{
	if (!started_)
		return false;

	// 发送数据,
	std::string request_data = "request data from DemoWebsocketClient";
	int nsend = websocket_->write(request_data.c_str());
	if (nsend < request_data.size())
	{
		printf("send not full, %d < %d\n", nsend, request_data.size());
	}

}

bool DemoWebsocketClient::Stop()
{
	if (!started_)
		return true;

	websocket_->close();
	return true;
}


void DemoWebsocketClient::ReadCallback(const std::string& msg)
{
	printf("ReadCallback msg: %s\n", msg.c_str());
}

void DemoWebsocketClient::DisconnectCallback(const std::string& msg)
{
	printf("DisconnectCallback msg: %s\n", msg.c_str());
}


