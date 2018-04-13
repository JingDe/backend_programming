#include"SocketsOps.h"
#include"Endian.h"

#include<netinet/in.h>
#include<arpa/inet.h>

#include"logging.muduo/Logging.h"

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr)
{
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr)
{
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
	// reinterpret_cast
}
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr)
{
	return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = hostToNetwork16(port);
	if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) // 将ipv4地址从文本形式转成二进制形式
		LOG_ERROR << "fromIpPort";
}

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = hostToNetwork16(port);
	if (inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) // 将ipv4地址从文本形式转成二进制形式
		LOG_ERROR << "fromIpPort";
}