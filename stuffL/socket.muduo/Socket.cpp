#include"Socket.h"
#include"InetAddress.h"

#include<strings.h>
#include<netinet/tcp.h>
#include<sys/socket.h>

Socket::~Socket()
{
	sockets::close(sockfd_);
}

bool Socket::getTcpInfo(struct tcp_info* tcpi) const
{
	socklen_t len = sizeof(*tcpi);
	bzero(tcpi, len);
	return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
	// int getsockopt(int socket, int level, int option_name, void* restrict option_value, socklen_t *restrict option_len);

}

bool Socket::getTcpInfoString(char* buf, int len) const
{
	struct tcp_info tcpi;
	bool ok = getTcpInfo(&tcpi);

}