#include"InetAddress.h"
#include"Endian.h"

#include<cstddef>

#include<strings.h>

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
	static_assert(offsetof(InetAddress, addr6_) == 0, "");
	static_assert(offsetof(InetAddress, addr_) == 0, "");
	if (ipv6)
	{
		bzero(&addr6_, sizeof addr6_);
		addr6_.sin6_family = AF_INET6;
		in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
		addr6_.sin6_addr = ip;
		addr6_.sin6_port = hostToNetwork16(port);
	}
	else
	{
		bzero(&addr_, sizeof addr_);
		addr_.sin_family = AF_INET;
		in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
		addr_.sin_addr.s_addr = hostToNetwork32(ip); //
		addr_.sin_port = hostToNetwork16(port);
	}
}

InetAddress::InetAddress(Slice ip, uint16_t port, bool ipv6)
{
	if (ipv6)
	{
		bzero(&addr6_, sizeof addr6_);
		fromIpPort(ip.data(), port, &addr6_);
	}
	else
	{
		bzero(&addr_, sizeof addr_);
		fromIpPort(ip.data(), port, &addr_);
	}
}

std::string InetAddress::toIpPort() const
{

}