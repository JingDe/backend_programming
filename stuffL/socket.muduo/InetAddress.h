#ifndef INETADDRESS_H_
#define INETADDRESS_H_

#include<netinet/in.h>

#include"SocketsOps.h"
#include"common/Slice.h"

class InetAddress {
public:
	explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
	InetAddress(Slice ip, uint16_t port, bool ipv6 = false);
	explicit InetAddress(const struct sockaddr_in& addr) :addr_(addr) {}
	explicit InetAddress(const struct sockaddr_in6& addr) :addr6_(addr) {}

	sa_family_t family() const { return addr_.sin_family; } // ��ַ��
	std::string toIp() const;
	std::string toIpPort() const;
	uint16_t toPort() const;

	// ת���sockaddr��ַ
	const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr6_); }
	void setSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }

	uint32_t ipNetEndian() const;
	uint16_t portNetEndian() const { return addr_.sin_port;  }

	static bool resolve(Slice hostname, InetAddress* result);

private:
	union {
		struct sockaddr_in addr_; // ��䵽struct sockaddr��С
		struct sockaddr_in6 addr6_;
	};
};

/*
struct sockaddr_in //�̶�INET_ADDRSTRLEN 16�ֽڴ�С
{
	__SOCKADDR_COMMON (sin_);   // ��ַ�� sa_family_t sin_family; typedef unsigned short int sa_family_t;
	in_port_t sin_port;			//�˿ں� typedef uint16_t in_port_t;
	struct in_addr sin_addr;	//ipv4��ַ 
								//typedef uint32_t in_addr_t;	struct in_addr{	in_addr_t s_addr;};		 				
	unsigned char sin_zero[sizeof(struct sockaddr) - 
			__SOCKADDR_COMMON_SIZE - sizeof(in_port_t) - sizeof(struct in_addr)]; 
			                    // ��䵽struct sockaddr��С
};

		

struct sockaddr_in6 // �̶�28�ֽ�
{
	__SOCKADDR_COMMON (sin6_);  // sa_family_t sin6_family; ���Ⱥ͵�ַ�幩2�ֽ�
	in_port_t sin6_port;	    //�����˿� 2���ֽ�
	uint32_t sin6_flowinfo;	    // IPV6����Ϣ 4���ֽ�
	struct in6_addr sin6_addr;	// IPV6��ַ 128λ 16�ֽ�
	uint32_t sin6_scope_id;	    // ��Χid 4�ֽ�
};


struct sockaddr // �̶�16�ֽ�
{
	__SOCKADDR_COMMON (sa_);	//��ַ��ͳ��� 2���ֽ�
	char sa_data[14];		    //��ַ
};
*/

#endif