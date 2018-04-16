#ifndef SOCKETOPS_H_
#define SOCKETOPS_H_

#include<arpa/inet.h>

namespace sockets {
	const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
	const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
	struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);

	const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
	const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

	void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
	void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

	void toIpPort(char* buf, size_t size, const struct sockaddr* add);
	void toIp(char* buf, size_t size, const struct sockaddr* addr);

	int createNonblockingOrDie(sa_family_t family);
	void close(int sockfd);
	void bindOrDie(int sockfd, const struct sockaddr* addr);
	void listenOrDie(int sockfd);
	int accept(int sockfd, struct sockaddr_in6* addr);

	void shutdownWrite(int sockfd);

	struct sockaddr_in6 getLocalAddr(int sockfd);
}

#endif

