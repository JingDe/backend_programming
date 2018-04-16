#include"InetAddress.h"
#include"Endian.h"

#include<cstddef>

#include<strings.h>
#include<netdb.h>

#include"logging.muduo/Logging.h"

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
		addr6_.sin6_port = sockets::hostToNetwork16(port);
	}
	else
	{
		bzero(&addr_, sizeof addr_);
		addr_.sin_family = AF_INET;
		in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
		addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip); //
		addr_.sin_port = sockets::hostToNetwork16(port);
	}
}

InetAddress::InetAddress(Slice ip, uint16_t port, bool ipv6)
{
	if (ipv6)
	{
		bzero(&addr6_, sizeof addr6_);
		sockets::fromIpPort(ip.data(), port, &addr6_);
	}
	else
	{
		bzero(&addr_, sizeof addr_);
		sockets::fromIpPort(ip.data(), port, &addr_);
	}
}


std::string InetAddress::toIpPort() const
{
	char buf[64] = "";
	sockets::toIpPort(buf, sizeof buf, getSockAddr());
	return buf; // 同 返回字符串字面量一样，只有一份string(buf)？？
}

std::string InetAddress::toIp() const
{
	char buf[64] = "";
	sockets::toIp(buf, sizeof buf, getSockAddr());
	return buf;
}

uint16_t InetAddress::toPort() const
{
	return sockets::networkToHost16(portNetEndian());
}

uint32_t InetAddress::ipNetEndian() const
{
	assert(family() == AF_INET);
	return addr_.sin_addr.s_addr;
}


/*
// Description of data base entry for a single host. 
struct hostent
{
	char *h_name;			// Official name of host.  
	char **h_aliases;		// Alias list.  
	int h_addrtype;		    // 主机地址类型 AF_INET AF_INET6  
	int h_length;			// 地址长度 
	char **h_addr_list;		// 地址列表 
#if defined __USE_MISC || defined __USE_GNU
# define	h_addr	h_addr_list[0]     // 第一个地址
#endif
};

*/
static __thread char t_resolveBuffer[64 * 1024];

// 解析主机名获得IP地址
bool InetAddress::resolve(Slice hostname, InetAddress* out)
{
	assert(out != NULL);
	struct hostent hent;
	struct hostent* he = NULL;
	int herrno = 0;
	bzero(&hent, sizeof hent);

	int ret = gethostbyname_r(hostname.data(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
	// int gethostbyname_r(const char* name, struct hostent *ret, char* buf, size_t buflen, struct hostent** result, int *h_errno);
	// glibc的可重入版本，不修改全局变量h_errno，提供h_errnop存储错误号
	if (ret == 0 && he != NULL)
	{
		assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
		out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}
	else
	{
		if (ret) // gethostbyname_r失败
			LOG_ERROR << "InetAddress::resolve";
		// else he为NULL，为查找到hostent
		return false;
	}
}