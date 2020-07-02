#include"wsclientdemo.h"

#include<string>

int main()
{
	std::string server_ip = "192.168.12.59";
	unsigned int server_port = 9312;
	std::string server_path = "/demotest";

	DemoWebsocketClient client(server_ip, server_port, server_path);
	client.Start();
	client.Run();
	sleep(3); // wait reply
	client.Stop();

	return 0;
}
