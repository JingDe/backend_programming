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
	if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) // ��ipv4��ַ���ı���ʽת�ɶ�������ʽ
		LOG_ERROR << "fromIpPort";
}

void sockets::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = hostToNetwork16(port);
	if (inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) // ��ipv4��ַ���ı���ʽת�ɶ�������ʽ
		LOG_ERROR << "fromIpPort";
}

// ��õ�ַ���ַ�����ʾ��ע��ת��������
void sockets::toIpPort(char* buf, size_t size, const struct sockaddr* addr)
{
	toIp(buf, size, addr);
	size_t end = ::strlen(buf);
	const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
	uint16_t port = sockets::networkToHost16(addr4->sin_port); // 16λ��������������
	assert(size > end);
	snprintf(buf + end, size - end, ":%u", port);
}

// ʹ��inet_ntopת��������IP���ַ�����ʽ
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

int sockets::createNonblockingOrDie(sa_family_t family)
{
#if VALGRIND
	int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
		LOG_FATAL << "sockets::createNonblockingOrDie";

	setNonBlockAndCloseOnExec(sockfd);
#else
	int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (sockfd < 0)
		LOG_FATAL << "sockets::createNonblockingOrDie";
#endif
	return sockfd;
}

void sockets::close(int sockfd)
{
	if (::close(sockfd) < 0)
		LOG_ERROR << "sockets::close";
}

void sockets::shutdownWrite(int sockfd)
{
	if (::shutdown(sockfd, SHUT_WR) < 0) // �رշ��Ͷ�
		LOG_ERROR << "sockets::shutdownWrite";
}

void sockets::bindOrDie(int sockfd, const struct sockaddr* addr)
{
	int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6))); // !!
	if (ret < 0)
		LOG_ERROR << "sockets::bindOrDie";
}

void sockets::listenOrDie(int sockfd)
{
	int ret = ::listen(sockfd, SOMAXCONN); // 128
	if (ret < 0)
		LOG_FATAL << "sockets::listenOrDie";
}

namespace
{
#if VALGRIND  ||  defined(NO_ACCEPT4)
	void setNonBlockAndCloseOnExec(int sockfd)
	{
		int flags = ::fcntl(sockfd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		int ret = ::fcntl(sockfd, F_SETFL, flags); 
		// �����ļ�״̬��ʶ��O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, O_NONBLOCK,�������ļ�����ģʽ���ļ�������־

		flags = ::fcntl(sockfd, F_GETFD, 0);
		flags |= FD_CLOEXEC;
		ret = ::fcntl(sockfd, F_SETFD, flags); // �����ļ���������ʶ

	}
#endif

}

// int accept(int sockfd, struct sockaddr* addr, socklen_t *addrlen);
// int accept4(int sockfd, struct sockaddr* addr, socklen_t *addrlen, int flags);
int sockets::accept(int sockfd, struct sockaddr_in6* addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);

#if VALGRIND  ||  defined(NO_ACCEPT4)
	int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
	setNonBlockAndCloseOnExec(connfd);
#else
	int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif

	if(connfd < 0)
	{
		int savedErrno = errno;
		LOG_ERROR << "Socket::accept";
		switch (savedErrno)
		{
		case EAGAIN:
		case ECONNABORTED:
		case EINTR:
		case EPROTO: // ???
		case EPERM:
		case EMFILE: // per-process lmit of open file desctiptor ???
					 // expected errors
			errno = savedErrno;
			break;
		case EBADF:
		case EFAULT:
		case EINVAL:
		case ENFILE:
		case ENOBUFS:
		case ENOMEM:
		case ENOTSOCK:
		case EOPNOTSUPP:
			// unexpected errors
			LOG_FATAL << "unexpected error of ::accept " << savedErrno;
			break;
		default:
			LOG_FATAL << "unknown error of ::accept " << savedErrno;
			break;
		}
	}
	return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr* addr)
{
	return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::readv(int sockfd, const struct iovec* iov, int iovcnt)
{
	return ::readv(sockfd, iov, iovcnt); // ���سɹ���ȡ���ֽ�����-1
}
// �������buffer��
// struct iovec{
//     void *iov_base;
//     size_t iov_len;
// };

struct sockaddr_in6 sockets::getLocalAddr(int sockfd)
{
	struct sockaddr_in6 localaddr;
	bzero(&localaddr, sizeof localaddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
	if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
		LOG_ERROR << "sockets::getLocalAddr";
	return localaddr;
}

int sockets::getSocketError(int sockfd)
{
	int optval;
	socklen_t optlen = sizeof optval;

	// ��ô���״̬�������
	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
	{
		return errno;
	}
	else
		return optval;
}

struct sockaddr_in6 sockets::getPeerAddr(int sockfd)
{
	struct sockaddr_in6 peeraddr;
	bzero(&peeraddr, sizeof peeraddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
	if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
	{
		LOG_ERROR << "sockets::getPeerAddr";
	}
	return peeraddr;
}

bool sockets::isSelfConnect(int sockfd)
{
	struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
	struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
	if (localaddr.sin6_family == AF_INET)
	{
		const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
		const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
		return laddr4->sin_port == raddr4->sin_port
			&& laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
	}
	else if (localaddr.sin6_family == AF_INET6)
	{
		return localaddr.sin6_port == peeraddr.sin6_port
			&& memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
	}
	else
	{
		return false;
	}
}