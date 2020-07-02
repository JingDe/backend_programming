#ifndef WEBSOCKET_DEMO_H_
#define WEBSOCKET_DEMO_H_

#include"WebsocketClient.h"

#include<memory>

class DemoWebsocketClient {
public:
	DemoWebsocketClient(const std::string& server_ip, unsigned int server_port, const std::string& server_path);
	~DemoWebsocketClient();

	void Start();
	void Run();
	void Stop();

	void ReadCallback(const std::string& msg);
	void DisconnectCallback(const std::string& msg);

private:
	std::string server_ip_;
	unsigned int server_port_;
	std::string server_path_;
	std::unique_ptr<WebsocketClient> websocket_;
	bool started_;
};

#endif