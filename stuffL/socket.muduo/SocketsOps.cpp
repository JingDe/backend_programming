#include"SocketsOps.h"
#include"Endian.h"

#include<cassert>

#include<unistd.h>
#include<netinet/in.h>

#include"logging.muduo/Logging.h"



const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr)
{
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr)
{
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
	// reinterpret_cast
}
struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in6* addr)
{
	return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr)
{
	return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}

const struct sockaddr_in6* sockets::sockaddr_in6_cast(const struct sockaddr* addr)
{
	return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}

void sockets::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = hostToNetwork16(port);
	if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) // 将ipv4地址从文本形式转成二进制形式
		LOG_ERROR << "fromIpPort";
}

void sockets::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = hostToNetwork16(port);
	if (inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) // 将ipv4地址从文本形式转成二进制形式
		LOG_ERROR << "fromIpPort";
}

void sockets::toIpPort(char* buf, size_t size, const struct sockaddr* addr)
{
	toIp(buf, size, addr);
	size_t end = ::strlen(buf);
	const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
	uint16_t port = sockets::networkToHost16(addr4->sin_port); // 16位网络大端序到主机序
	assert(size > end);
	snprintf(buf + end, size - end, ":%u", port);
}

void sockets::toIp(char* buf, size_t size, const struct sockaddr* addr)
{
	if (addr ->sa_family == AF_INET)
	{
		assert(size >= INET_ADDRSTRLEN); // 16
		const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
		::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
		// const char* inet_ntop(int af, const void* src, char* dst, socklen_t size);
	}
	else if (addr->sa_family == AF_INET6)
	{
		assert(size >= INET6_ADDRSTRLEN);// 46
		const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
		::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
	}
}

void sockets::close(int sockfd)
{
	if (::close(sockfd) < 0)
		LOG_ERROR << "sockets::close";
}